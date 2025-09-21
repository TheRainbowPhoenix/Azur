//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//
// azur.resources: Access to embedded resources

#pragma once
#include <azur/defs.h>

namespace azur {

/* Information about a single embedded file */
struct ResourceGroupEntry {
    void const *data;
    size_t size;
};

/* Index for a resource group */
using ResourceGroup = map<string, ResourceGroupEntry>;

/* Register a resource group (shared globally). This is normally called by
   groups in auto-generates code, never by users. */
void registerResourceGroup(char const *name, ResourceGroup const &group);

/* Get a resource using the identifier "@name:path". */
void const *getResource(char const *prefixedPath, size_t *size);

/* Debug function: dump all resources in log. */
void dumpAllResources();

} /* namespace azur */
