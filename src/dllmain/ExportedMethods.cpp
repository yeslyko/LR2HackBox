#include <LR2HackBox/LR2HackBox.hpp>
#include "../LR2HackBox/Features/Numbers.hpp"

extern "C" __declspec(dllexport) Numbers::Judgements __cdecl GetJudgements() {
	Numbers& numbers = *(Numbers*)LR2HackBox::Get().mNumbers.get();
	return numbers.GetTotalJudgements();
}