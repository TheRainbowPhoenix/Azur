//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//

#include <azur/resources.h>
#include <azur/log.h>
#include <string.h>

namespace azur {

// TODO: Avoid global constructors for resource group registrations
using GroupStorage = map<string, ResourceGroup const *>;
static GroupStorage *resourceGroups = nullptr;

void registerResourceGroup(char const *name, ResourceGroup const &group)
{
    if(!resourceGroups)
        resourceGroups = new GroupStorage();

    // TODO: Check for resource group name conflicts at runtime?
    resourceGroups->insert({name, &group});
}

void const *getResource(char const *prefixedPath, size_t *size)
{
    if(size)
        *size = 0;

    char const *p = prefixedPath;
    if(!p || *p++ != '@')
        return nullptr;

    char const *colon = strchr(p, ':');
    if(!colon)
        return nullptr;

    string groupName(p, colon - p);
    string path(colon + 1);

    auto it = resourceGroups->find(groupName);
    if(it == resourceGroups->end())
        return nullptr;

    // TODO: Normalize path in getResource()?
    ResourceGroup const *RG = it->second;
    auto it2 = RG->find(path);
    if(it2 == RG->end())
        return nullptr;

    ResourceGroupEntry const &RGE = it2->second;
    if(size)
        *size = RGE.size;
    return RGE.data;
}

void dumpAllResources()
{
    if(!resourceGroups || !resourceGroups->size()) {
        azlog(DEBUG, "No registered resource groups\n");
        return;
    }

    for(auto const &it: *resourceGroups) {
        azlog(DEBUG, "Resource group '%s':\n", it.first.c_str());
        ResourceGroup const * RG = it.second;

        if(!RG->size()) {
            azlog(DEBUG, "  No resources in group\n");
            continue;
        }

        for(auto const &it2: *RG) {
            ResourceGroupEntry const &RGE = it2.second;
            azlog(DEBUG, "  '%s': %p (%zu)\n", it2.first.c_str(),
                RGE.data, RGE.size);
        }
    }
}

} /* namespace azur */
