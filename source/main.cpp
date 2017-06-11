#include "c4d.h"
#include <string.h>
#include "main.h"


Bool PluginStart(void)
    {
    if (!RegisterGraphAxisConstraint())
        return false;

    return true;
    }

void PluginEnd(void)
    {
    }

Bool PluginMessage(Int32 id, void* data)
    {
    switch (id)
        {
        case C4DPL_INIT_SYS:
            if (!resource.Init())
                return FALSE;
            return TRUE;
        }
    return false;
    }
