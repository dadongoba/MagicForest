#pragma once

// The classifier proposes an interpretation; native code verifies its evidence
// belongs to an earlier NPC message and the current player turn.
namespace ForestCommissionAcceptance
{
inline bool EvidenceIsValid(bool Accepted, int Offer, int Accept, int Count,
    bool OfferIsNpc, bool AcceptIsPlayer, bool OfferQuoteMatches, bool AcceptQuoteMatches)
{
    return Accepted && Count >= 2 && Offer >= 0 && Offer < Accept &&
        Accept == Count - 1 && OfferIsNpc && AcceptIsPlayer &&
        OfferQuoteMatches && AcceptQuoteMatches;
}
}
