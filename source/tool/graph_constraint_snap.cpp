/*
coded by graphos
contact => graphos.xyz
*/

#include "graph_constraint_snap.h"
#include <string.h>

AxisConstraint::AxisConstraint()
{
    _snap = nullptr;
}

AxisConstraint::~AxisConstraint()
{
    if (_snap)
        SnapCore::Free(_snap);
    _snap = nullptr;
}

void AxisConstraint::InitDefaultSettings(BaseDocument* doc, BaseContainer& data)
{
    DescriptionToolData::InitDefaultSettings(doc, data);
    data.SetBool(1002, true);
    data.SetBool(1003, true);
    data.SetBool(1004, true);


    //Enable local snap settings for the tool
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

	Int32	AxisConstraint::GetState(BaseDocument *doc)
{
    return CMD_ENABLED;
}

    Bool	AxisConstraint::GetCursorInfo(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, Float x, Float y, BaseContainer& bc)
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
    Vector		startPos = bd->SW(Vector(x, y, 500));
    SnapResult	snapResul = SnapResult();

    _snap->Snap(startPos, snapResul, SNAPFLAGS_0);
    // in case of single click tool you can use the filled SnapResult to update realtime preview
    return true;
}

    Bool    AxisConstraint::MouseInput(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
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
    case BFM_INPUT_MOUSELEFT: button = KEY_MLEFT; break;
    case BFM_INPUT_MOUSERIGHT: button = KEY_MRIGHT; break;
    default: return true;
    }
    // check if we got an object selected and op is valid
    doc->GetActiveObjects(*objSelectedList, GETACTIVEOBJECTFLAGS_CHILDREN);
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
    if (_snap->Snap(startPos, snapResul, SNAPFLAGS_0) && snapResul.snapmode != NOTOK)
        startPos = snapResul.mat.off;

    // use snap matrix for inferred guide to use the correct orientation
    if (snapResul.snapmode != NOTOK)
        guideMatrix = snapResul.mat;
    else
        guideMatrix.off = startPos;

    // add an inferred guide to start point this show its effect only if dynamic guide snap is on
    _snap->AddInferred(doc, guideMatrix, INFERRED_GUIDE_POINT);

    win->MouseDragStart(button, mouseX, mouseY, MOUSEDRAGFLAGS_DONTHIDEMOUSE | MOUSEDRAGFLAGS_NOMOVE);
    while (win->MouseDrag(&dx, &dy, &device) == MOUSEDRAGRESULT_CONTINUE)
    {

        mouseX += dx;
        mouseY += dy;
        snapResul = SnapResult();
        destPos = bd->SW(Vector(mouseX, mouseY, bd->WS(startPos).z));

        if (_snap->Snap(destPos, snapResul, SNAPFLAGS_IGNORE_SELECTED) && snapResul.snapmode != NOTOK)
        {
            // If mouse dont move we check if we need to select or not
            if (dx == 0.0 && dy == 0.0)
                {
                // if we are in point snap mode
                if (snapResul.snapmode == SNAPMODE_POINT){

                    // if the target object is selected
                    if (objSelectedList->Find(snapResul.target) != NOTOK)
                    {
                        BaseContainer* kb_state = NewObjClear(BaseContainer);
                        if (!win->BfGetInputEvent(BFM_INPUT_KEYBOARD, kb_state))
                            _shift = false;

                        if (kb_state->GetInt32(BFM_INPUT_QUALIFIER) & QSHIFT)
                            _shift = true;
                        else
                            _shift = false;

                        PointObject* pobj = ToPoint(snapResul.target);
                        BaseSelect* bs = pobj->GetPointS();

                        if (_shift)
                        {
                            bs->Toggle(snapResul.component);
                        }
                        else
                        {
                            bs->DeselectAll();
                            bs->Select(snapResul.component);
                        }

                        DrawViews(DRAWFLAGS_ONLY_ACTIVE_VIEW | DRAWFLAGS_NO_THREAD | DRAWFLAGS_NO_ANIMATION);
                    }
                }
                continue;
            }

            // if mouse move
            else{
                if (data.GetBool(1002, true) || data.GetBool(1003, true) || data.GetBool(1004, true))
                {
                    // get world coordinate of cibled points
                    const Vector* bufferReadArray = ToPoint(snapResul.target)->GetPointR();
                    BaseObject* cibledObj = (BaseObject*)snapResul.target;
                    Vector cibleWorldPos = cibledObj->GetMg() * bufferReadArray[snapResul.component];

                    for (Int i = 0; i < objSelectedList->GetCount(); i++)
                    {
                        BaseObject*	op = nullptr;
                        op = (BaseObject*)objSelectedList->GetIndex(i);
                        PointObject* pobj = ToPoint(op);
                        const Vector* pts = pobj->GetPointR();
                        BaseSelect* bs = pobj->GetPointS();
                        Vector* points = pobj->GetPointW();

                        if (doc->GetMode() == Mpoints){
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

                                    doc->AddUndo(UNDOTYPE_CHANGE, op);
                                    points[y] = destPos;
                                    op->Message(MSG_UPDATE);
                                }
                            }
                        }else if (doc->GetMode() == Mobject || doc->GetMode() == Mmodel){
                            // Get the actual world global Matrix
                            Matrix m = op->GetMg();
                            destPos = m.off;

                            //X
                            if (data.GetBool(1002, true))
                                destPos.x = cibleWorldPos.x;

                            // Y
                            if (data.GetBool(1003, true))
                                destPos.y = cibleWorldPos.y;

                            // Z
                            if (data.GetBool(1004, true))
                                destPos.z = cibleWorldPos.z;

                            doc->AddUndo(UNDOTYPE_CHANGE, op);
                            m.off = destPos;
                            op->SetMg(m);
                         }
                    }
                }
                DrawViews(DRAWFLAGS_ONLY_ACTIVE_VIEW | DRAWFLAGS_NO_THREAD | DRAWFLAGS_NO_ANIMATION);
            }
        }
    }

    if (win->MouseDragEnd() == MOUSEDRAGRESULT_ESCAPE)
        doc->DoUndo();

    // flush dynamc guides to clean the screen
    _snap->FlushInferred();

    objSelectedList->Flush();
    EventAdd();
    return true;
}
void AxisConstraint::FreeTool(BaseDocument* doc, BaseContainer& data)
{
    if (_snap)
        SnapCore::Free(_snap);
    _snap = nullptr;
}

Bool RegisterGraphAxisConstraint()
{
return RegisterToolPlugin(ID_GRAPH_AXIS_CONSTRAINT, "Axis Constraint Tool", 0, AutoBitmap("snapXYZ.png"), "Axis Constraint Tool", NewObjClear(AxisConstraint));
}