#ifndef GRAPH_CONSTRAINT_SNAP_H__
#define GRAPH_CONSTRAINT_SNAP_H__

#include "c4d.h"
#include "c4d_snapdata.h"

#define ID_GRAPH_AXIS_CONSTRAINT 1038076

class AxisConstraint : public DescriptionToolData
{
public:
                            AxisConstraint();
    virtual                 ~AxisConstraint();
    Int32                 	GetToolPluginId() { return ID_GRAPH_AXIS_CONSTRAINT; }
    Bool                    isShiftPressed(EditorWindow* win);
    Int32					GetState(BaseDocument* doc);
    Bool					GetCursorInfo(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, Float x, Float y, BaseContainer& bc);
    const String			GetResourceSymbol() { return String("graph_constraint_snap"); }
    void					InitDefaultSettings(BaseDocument* doc, BaseContainer& data);
    Bool					MouseInput(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg);
    void					FreeTool(BaseDocument* doc, BaseContainer& data);

private:
    SnapCore* _snap;
};

#endif