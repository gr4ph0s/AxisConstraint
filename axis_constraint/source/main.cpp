#include "c4d.h"
#include "c4d_snapdata.h"
#include "c4d_symbols.h"

#define ID_GRAPH_AXIS_CONSTRAINT 1038076

class AxisConstraintTool : public DescriptionToolData
{
public:
	AxisConstraintTool() {};
	~AxisConstraintTool()
	{ 
		if (_snap != nullptr)
			SnapCore::Free(_snap);
	};

	Int32 GetToolPluginId()	{ return ID_GRAPH_AXIS_CONSTRAINT; }
	const String GetResourceSymbol() { return "graph_constraint_snap"_s; }
	Int32 GetState(BaseDocument* doc) { return CMD_ENABLED; };

	void InitDefaultSettings(BaseDocument* doc, BaseContainer& data)
	{
		DescriptionToolData::InitDefaultSettings(doc, data);
    data.SetBool(1002, true);
    data.SetBool(1003, true);
    data.SetBool(1004, true);

    // Enable local snap settings for the tool
    BaseContainer psnap;
    psnap.SetBool(SNAP_SETTINGS_ENABLED, true);

    BaseContainer psnapFalse;
    psnapFalse.SetBool(SNAP_SETTINGS_ENABLED, false);

    BaseContainer snap_settings;
    snap_settings.SetBool(SNAP_SETTINGS_ENABLED, true);
		snap_settings.SetFloat(SNAP_SETTINGS_RADIUS, 10.0);
    snap_settings.SetContainer(SNAPMODE_POINT, psnap);
    snap_settings.SetContainer(SNAPMODE_EDGE, psnapFalse);

    data.SetContainer(SNAP_SETTINGS, snap_settings);
	}

	Bool GetCursorInfo(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, Float x, Float y, BaseContainer& bc)
	{
		if (bc.GetId() == BFM_CURSORINFO_REMOVE)
			return true;

		// allocate snap if necessary
		if (!_snap)
		{
			_snap = SnapCore::Alloc();
			if (!_snap)
				return false;
		}

		// initialize snap always
		if (!_snap->Init(doc, bd))
			return false;

		// just use snap in get GetCursorInfo if you need realtime snap drawing or if you need to snap at single click like in guide tool or knife tool
		Vector startPos = bd->SW(Vector(x, y, 500));
		SnapResult	snapResul = SnapResult();

		_snap->Snap(startPos, snapResul, SNAPFLAGS::NONE);
		// in case of single click tool you can use the filled SnapResult to update realtime preview
		return true;
	}
	
	Bool MouseInput(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
	{
		BaseContainer   bc;
		BaseContainer   device;
		Int32           button;
		Float           dx, dy;
		Float           mouseX = msg.GetFloat(BFM_INPUT_X);
		Float           mouseY = msg.GetFloat(BFM_INPUT_Y);

		SnapResult      snapResul = SnapResult();
		Vector          startPos = bd->SW(Vector(mouseX, mouseY, 500));
		Vector          destPos;

		Matrix          guideMatrix = Matrix();

		AutoAlloc<AtomArray>objSelectedList;
		if (!objSelectedList)
			return false;

		switch (msg.GetInt32(BFM_INPUT_CHANNEL))
		{
			case BFM_INPUT_MOUSELEFT: 
			{
				button = KEY_MLEFT;
				break;
			}
			case BFM_INPUT_MOUSERIGHT: 
			{
				button = KEY_MRIGHT;
				break;
			}
			default:
				return true;
		}
		// check if we got an object selected and op is valid
		doc->GetActiveObjects(*objSelectedList, GETACTIVEOBJECTFLAGS::CHILDREN);
		if (objSelectedList->GetCount() < 1)
			return true;

		// allocate snap if necessary
		if (!_snap)
		{
			_snap = SnapCore::Alloc();
			if (!_snap)
			{
				objSelectedList->Flush();
				return false;
			}
		}

		// initialize snap always
		if (!_snap->Init(doc, bd))
		{
			objSelectedList->Flush();
			return false;
		}

		// if snap modify initial position do not pass SNAPFLAGS_IGNORE_SELECTED otherwise selected object will be not evaluated
		if (_snap->Snap(startPos, snapResul, SNAPFLAGS::NONE) && snapResul.snapmode != NOTOK)
			startPos = snapResul.mat.off;

		// use snap matrix for inferred guide to use the correct orientation
		if (snapResul.snapmode != NOTOK)
			guideMatrix = snapResul.mat;
		else
			guideMatrix.off = startPos;

		// add an inferred guide to start point this show its effect only if dynamic guide snap is on
		_snap->AddInferred(doc, guideMatrix, INFERREDGUIDETYPE::POINT);

		win->MouseDragStart(button, mouseX, mouseY, MOUSEDRAGFLAGS::DONTHIDEMOUSE | MOUSEDRAGFLAGS::NOMOVE);
		while (win->MouseDrag(&dx, &dy, &device) == MOUSEDRAGRESULT::CONTINUE)
		{

			mouseX += dx;
			mouseY += dy;
			snapResul = SnapResult();
			destPos = bd->SW(Vector(mouseX, mouseY, bd->WS(startPos).z));

			if (_snap->Snap(destPos, snapResul, SNAPFLAGS::NONE) && snapResul.snapmode != NOTOK)
			{
				// If mouse dont move we check if we need to select or not
				if (dx == 0.0 && dy == 0.0)
				{
					// if we are in point snap mode
					if (snapResul.snapmode == SNAPMODE_POINT) 
					{
						PointObject* pobj = ToPoint(snapResul.target);
						BaseSelect* bs = pobj->GetPointS();

						// if the target object is selected
						if (objSelectedList->Find(snapResul.target) != NOTOK)
						{
							if (this->IsShiftPressed(win))
							{
								bs->Toggle(snapResul.component);
							}
							else
							{
								bs->DeselectAll();
								bs->Select(snapResul.component);
							}
						}
					}
					DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);
					continue;
				}

				// if mouse move
				else 
				{
					if (data.GetBool(1002, true) || data.GetBool(1003, true) || data.GetBool(1004, true))
					{
						// get world coordinate of cibled points
						const Vector* bufferReadArray = ToPoint(snapResul.target)->GetPointR();
						BaseObject* cibledObj = (BaseObject*)snapResul.target;
						Vector cibleWorldPos = cibledObj->GetMg() * bufferReadArray[snapResul.component];

						for (Int32 i = 0; i < objSelectedList->GetCount(); i++)
						{
							BaseObject* op = nullptr;
							op = (BaseObject*)objSelectedList->GetIndex(i);
							PointObject* pobj = ToPoint(op);
							const Vector* pts = pobj->GetPointR();
							BaseSelect* bs = pobj->GetPointS();
							Vector* points = pobj->GetPointW();

							if (doc->GetMode() == Mpoints) 
							{
								for (int y = 0; y < pobj->GetPointCount(); y++)
								{
									if (bs->IsSelected(y))
									{
										// get the selected point in global space
										Vector pointInGlobalSpace = op->GetMg() * pts[y];

										// X
										if (data.GetBool(1002, true))
											pointInGlobalSpace.x = cibleWorldPos.x;

										// Y
										if (data.GetBool(1003, true))
											pointInGlobalSpace.y = cibleWorldPos.y;

										// Z
										if (data.GetBool(1004, true))
											pointInGlobalSpace.z = cibleWorldPos.z;

										// Get the world point in local space
										destPos = ~op->GetMg() * pointInGlobalSpace;

										doc->AddUndo(UNDOTYPE::CHANGE, op);
										points[y] = destPos;
										op->Message(MSG_UPDATE);
									}
								}
							}
							else if (doc->GetMode() == Mobject || doc->GetMode() == Mmodel) 
							{
								// Get the actual world global Matrix
								Matrix m = op->GetMg();
								destPos = m.off;

								// X
								if (data.GetBool(1002, true))
									destPos.x = cibleWorldPos.x;

								// Y
								if (data.GetBool(1003, true))
									destPos.y = cibleWorldPos.y;

								// Z
								if (data.GetBool(1004, true))
									destPos.z = cibleWorldPos.z;

								doc->AddUndo(UNDOTYPE::CHANGE, op);
								m.off = destPos;
								op->SetMg(m);
							}
						}
					}
					DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);
				}
			}
		}

		if (win->MouseDragEnd() == MOUSEDRAGRESULT::ESCAPE)
		{
			doc->DoUndo();
		}

		// flush dynamic guides to clean the screen
		_snap->FlushInferred();

		objSelectedList->Flush();
		EventAdd();
		return true;
	}

	void FreeTool(BaseDocument* doc, BaseContainer& data)
	{
		if (_snap != nullptr)
			SnapCore::Free(_snap);
	}

private:
	SnapCore* _snap = nullptr;

	Bool IsShiftPressed(EditorWindow* win)
	{
		BaseContainer kb_state;
		if (!win->BfGetInputEvent(BFM_INPUT_KEYBOARD, &kb_state))
			return false;

		return kb_state.GetInt32(BFM_INPUT_QUALIFIER) & QSHIFT;
	}
};



Bool PluginStart()
{
	return RegisterToolPlugin(ID_GRAPH_AXIS_CONSTRAINT, "Axis Constraint Tool"_s, 0, AutoBitmap("snapXYZ.png"_s), "Axis Constraint Tool"_s, NewObjClear(AxisConstraintTool));
}

void PluginEnd()
{
}

Bool PluginMessage(Int32 id, void* data)
{
	switch (id)
	{
		case C4DPL_INIT_SYS:
		{
			if (!g_resource.Init())
				return false;		// don't start plugin without resource

			return true;
		}
	}

	return true;
}
