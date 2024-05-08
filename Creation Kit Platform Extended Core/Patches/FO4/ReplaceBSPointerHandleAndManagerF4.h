﻿// Copyright © 2023-2024 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/gpl-3.0.html

#pragma once

#include "Core/Module.h"
#include "Core/Relocator.h"
#include "Core/RelocationDatabase.h"

namespace CreationKitPlatformExtended
{
	namespace Patches
	{
		namespace Fallout4
		{
			using namespace CreationKitPlatformExtended::Core;
			
			class ReplaceBSPointerHandleAndManagerPatch : public Module
			{
			public:
				ReplaceBSPointerHandleAndManagerPatch();

				virtual bool HasOption() const;
				virtual bool HasCanRuntimeDisabled() const;
				virtual const char* GetOptionName() const;
				virtual const char* GetName() const;
				virtual bool HasDependencies() const;
				virtual Array<String> GetDependencies() const;

				bool IsVersionValid(const RelocationDatabaseItem* lpRelocationDatabaseItem) const;
				bool Install_163(const Relocator* lpRelocator, const RelocationDatabaseItem* lpRelocationDatabaseItem,
					bool Extremly);
				bool Install_980(const Relocator* lpRelocator, const RelocationDatabaseItem* lpRelocationDatabaseItem,
					bool Extremly);

				static void IncRefPatch();
				static void DecRefPatch();
			protected:
				virtual bool QueryFromPlatform(EDITOR_EXECUTABLE_TYPE eEditorCurrentVersion,
					const char* lpcstrPlatformRuntimeVersion) const;
				virtual bool Activate(const Relocator* lpRelocator, const RelocationDatabaseItem* lpRelocationDatabaseItem);
				virtual bool Shutdown(const Relocator* lpRelocator, const RelocationDatabaseItem* lpRelocationDatabaseItem);
			private:
				ReplaceBSPointerHandleAndManagerPatch(const ReplaceBSPointerHandleAndManagerPatch&) = default;
				ReplaceBSPointerHandleAndManagerPatch& operator=(const ReplaceBSPointerHandleAndManagerPatch&) = default;
			};
		}
	}
}