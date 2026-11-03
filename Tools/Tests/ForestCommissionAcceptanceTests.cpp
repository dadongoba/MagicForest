#include "../../Source/demo3_UE58/ForestCommissionAcceptance.h"
#include <cassert>
#include <iostream>

int main()
{
    using ForestCommissionAcceptance::EvidenceIsValid;
    assert(EvidenceIsValid(true, 0, 1, 2, true, true, true, true));
    assert(EvidenceIsValid(true, 1, 7, 8, true, true, true, true));
    assert(!EvidenceIsValid(false, 0, 1, 2, true, true, true, true));
    // A past player promise is not a new acceptance, even if its quote is real.
    assert(!EvidenceIsValid(true, 0, 1, 4, true, true, true, true));
    // The NPC cannot accept its own commission; nor can a fabricated role tag.
    assert(!EvidenceIsValid(true, 0, 1, 2, false, true, true, true));
    assert(!EvidenceIsValid(true, 0, 1, 2, true, false, true, true));
    // A reply generated after the player's message cannot establish an earlier offer.
    assert(!EvidenceIsValid(true, 1, 0, 2, true, true, true, true));
    assert(!EvidenceIsValid(true, 1, 1, 2, true, true, true, true));
    assert(!EvidenceIsValid(true, -1, 1, 2, true, true, true, true));
    assert(!EvidenceIsValid(true, 0, 1, 1, true, true, true, true));
    // Unmatched or empty model-supplied quotes cannot create authoritative state.
    assert(!EvidenceIsValid(true, 0, 1, 2, true, true, false, true));
    assert(!EvidenceIsValid(true, 0, 1, 2, true, true, true, false));
    std::cout << "Acceptance evidence checks passed (12 cases)\n";
}
