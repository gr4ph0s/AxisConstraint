#ifndef GRAPH_CONSTRAINT_SNAP_H__
#define GRAPH_CONSTRAINT_SNAP_H__

#include "c4d.h"
#include "c4d_snapdata.h"

#define ID_GRAPH_SNAPTOOL 1038076

class AxisConstraint : public DescriptionToolData
{
public:
                            AxisConstraint::AxisConstraint();
    virtual                 AxisConstraint::~AxisConstraint();
    Int32                   AxisConstraint::GetToolPluginId() { return ID_GRAPH_SNAPTOOL; }
    Int32					AxisConstraint::GetState(BaseDocument* doc);
    Bool					AxisConstraint::GetCursorInfo(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, Float x, Float y, BaseContainer& bc);
    const String			AxisConstraint::GetResourceSymbol() { return String("graph_constraint_snap"); }
    void					AxisConstraint::InitDefaultSettings(BaseDocument* doc, BaseContainer& data);
    Bool					AxisConstraint::MouseInput(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg);
    Bool                    AxisConstraint::KeyboardInput(const SnapStruct& ss, BaseDocument* doc, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg);
    void					AxisConstraint::FreeTool(BaseDocument* doc, BaseContainer& data);

private:
    SnapCore* AxisConstraint::_snap;
    Bool _shift;
};

#endif