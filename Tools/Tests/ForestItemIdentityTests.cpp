// Standalone regression check. No Unreal runtime, renderer, assets or network.
#include "ForestItemIdentity.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

struct Item {std::string Id,InventoryKey;int Quantity;};
int main()
{
 using ForestItemIdentity::ConflictsWithOtherItem;
 std::vector<Item> Ledger={{"saved-regional-item","old-shell-key",4}};
 auto& Migrating=Ledger.front();
 const std::string OriginalKey=Migrating.InventoryKey;
 Migrating.InventoryKey="rain-bottle-water3";
 // Regression: a row renamed in place must not collide with itself forever.
 assert(!ConflictsWithOtherItem(Migrating,Ledger));
 assert(Migrating.Id=="saved-regional-item" && Migrating.Quantity==4);
 Migrating.InventoryKey=OriginalKey;
 assert(Ledger.size()==1 && Migrating.Quantity==4);

 const Item NewItem={"fresh-guid","old-shell-key",1};
 assert(ConflictsWithOtherItem(NewItem,Ledger)); // Other owner still conflicts.
 const Item Unique={"fresh-guid","new-unique-key",1};
 assert(!ConflictsWithOtherItem(Unique,Ledger));
 const Item SameIdentityCopy={"saved-regional-item","old-shell-key",4};
 assert(!ConflictsWithOtherItem(SameIdentityCopy,Ledger));

 Ledger.push_back({"different-item","old-shell-key",2});
 assert(ConflictsWithOtherItem(SameIdentityCopy,Ledger));
 assert(Ledger[0].Quantity==4 && Ledger[1].Quantity==2);
 std::cout<<"PASS: self-identity exclusion, other-owner collisions, stable quantities\n";
}
