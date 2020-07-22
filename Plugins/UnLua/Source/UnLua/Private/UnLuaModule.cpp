// Tencent is pleased to support the open source community by making UnLua available.
// 
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the MIT License (the "License"); 
// you may not use this file except in compliance with the License. You may obtain a copy of the License at
//
// http://opensource.org/licenses/MIT
//
// Unless required by applicable law or agreed to in writing, 
// software distributed under the License is distributed on an "AS IS" BASIS, 
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and limitations under the License.

#include "Modules/ModuleManager.h"
#include "LuaContext.h"
#include "UnLua.h"

#define LOCTEXT_NAMESPACE "FUnLuaModule"

class FUnLuaModule : public IModuleInterface, private FSelfRegisteringExec
{
public:
    virtual void StartupModule() override
    {
        FLuaContext::Create();
        GLuaCxt->RegisterDelegates();
    }

    virtual void ShutdownModule() override
    {
    }

    // FExec
    virtual bool Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override
    {
        int nResult = 0;
        UnLua::FLuaRetValues Ret = UnLua::CallTableFunc(UnLua::GetState(), "SystemCallback", "OnGmCmd", (ANSICHAR*)StringCast<ANSICHAR>(static_cast<const TCHAR*>(Cmd)).Get());
        return Ret.IsValid() && Ret.Num() >= 1 && Ret[0].Value<bool>();
    }
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnLuaModule, UnLua)
