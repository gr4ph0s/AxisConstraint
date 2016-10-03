// This is the main file of the CINEMA 4D SDK
//
// When you create your own projects much less code is needed (this file is rather long as it tries to show all kinds of different uses).
//
// An empty project simply looks like this:
//
// #include "c4d.h"
//
// Bool PluginStart(void)
// {
//   ...do or register something...
//   return true;
// }
//
// void PluginEnd(void)
// {
// }
//
// Bool PluginMessage(Int32 id, void *data)
// {
//   return false;
// }
//

#include "c4d.h"
#include <string.h>
#include "main.h"


Bool PluginStart(void)
{
	VERSIONTYPE versionType = GeGetVersionType();

	if (!RegisterSnapTool())
		return false;
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
			return FALSE; // don't start plugin without resource 
		return TRUE;
	}
	return false;
}
