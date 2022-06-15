#pragma once

#include <cstdint>
#include "Math.hpp"

#include "TDBVer.hpp"

#pragma pack(push, r1, 1)
#ifdef DMC5
#include "ReClass_Internal_DMC5.hpp"
#elif defined(MHRISE)
#include "ReClass_Internal_RE8.hpp"
#elif defined(RE8)
#include "ReClass_Internal_RE8.hpp"
#elif RE3
#include "ReClass_Internal_RE3_TDB70.hpp"
#elif RE2
#include "ReClass_Internal_RE2_TDB70.hpp"
#elif RE7
#include "ReClass_Internal_RE2_TDB70.hpp"
#endif
#pragma pack(pop, r1)

#include "Enums_Internal.hpp"

#include "RETypes.hpp"
#include "REType.hpp"
#include "RETypeCLR.hpp"
#include "RETypeDB.hpp"
#include "RETypeDefinition.hpp"
#include "REArray.hpp"
#include "REContext.hpp"
#include "REManagedObject.hpp"
#include "SystemArray.hpp"
#include "REString.hpp"
#include "RETransform.hpp"
#include "REComponent.hpp"
#include "RopewaySweetLightManager.hpp"
#include "REVariableDescriptor.hpp"
#include "REGlobals.hpp"
