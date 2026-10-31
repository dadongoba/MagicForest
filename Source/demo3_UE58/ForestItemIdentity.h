#pragma once

// Kept independent of Unreal so the save-migration identity rule can be checked
// without starting the editor or loading a game world.
namespace ForestItemIdentity
{
 template<typename Item,typename Range>
 bool ConflictsWithOtherItem(const Item& Candidate,const Range& Rows)
 {
  for(const auto& Existing:Rows)
   if(Existing.Id!=Candidate.Id && Existing.InventoryKey==Candidate.InventoryKey)return true;
  return false;
 }
}
