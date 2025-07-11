#include "BaseModels/ModFeature.hpp"

bool ModFeature::EarlyInit(uintptr_t moduleBase) {
	ModFeature::mModuleBase = moduleBase;
	return true;
}

bool ModFeature::Init(uintptr_t moduleBase) {
	ModFeature::mModuleBase = moduleBase;
	return true;
}

bool ModFeature::Deinit() {
	return true;
}