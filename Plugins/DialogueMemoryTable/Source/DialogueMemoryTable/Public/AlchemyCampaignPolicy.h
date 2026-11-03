#pragma once

// Portable save-transition policy shared with the native regression test.
namespace AlchemyCampaign
{
constexpr int LastDay = 3;
struct State { int Day = 1; int Confirmed = 0; bool Completed = false; };
inline State Normalize(State S)
{
    if (S.Day > LastDay || S.Confirmed >= LastDay || S.Completed)
        return {LastDay, LastDay, true};
    if (S.Day < 1) S.Day = 1;
    if (S.Confirmed < 0) S.Confirmed = 0;
    if (S.Day <= S.Confirmed) S.Day = S.Confirmed + 1;
    return S;
}
inline bool Confirm(State& S, int ReceiptDay)
{
    if (ReceiptDay < 1 || ReceiptDay > LastDay) return false;
    if (ReceiptDay <= S.Confirmed) return true;
    if (S.Completed || ReceiptDay != S.Day) return false;
    S.Confirmed = ReceiptDay;
    if (ReceiptDay == LastDay) S.Completed = true;
    else ++S.Day;
    return true;
}
inline bool AllDailyCommissionsResolved(int Day, bool Blacksmith, bool Operator, bool Novelist)
{
    return Day >= 1 && Day <= LastDay && Blacksmith && Operator && Novelist;
}
inline int CommissionId(int Offset, int Day)
{
    return Offset >= 0 && Day >= 1 && Day <= LastDay ? Offset + Day : 0;
}
}
