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

#include "UnLuaPrivate.h"
#include "Misc/FileHelper.h"
#include "Engine/Blueprint.h"
#include "Blueprint/UserWidget.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IPluginManager.h"

FString GetEmmyLuaDeclareString(UClass* Class)
{
    FString DeclareString;
    for (TFieldIterator<FObjectPropertyBase> PropertyIt(Class, EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::ExcludeDeprecated); PropertyIt; ++PropertyIt)
    {
        FObjectPropertyBase* obj = *PropertyIt;
        if (obj)
        {
            UClass* WidgetClass = obj->PropertyClass;
            FString DeclareCode = FString::Printf(TEXT("---@field public %s %s%s\n"), *obj->GetName(), WidgetClass->GetPrefixCPP(), *WidgetClass->GetName());
            DeclareString += DeclareCode;
        }
    }
    return DeclareString;
}

bool UpdatePropertyDeclare(FString& Content, FString& DeclareString)
{
    if (DeclareString.Len() > 0)
    {
        int32 nStartLineBegin = Content.Find(TEXT("---@class"), ESearchCase::CaseSensitive, ESearchDir::FromStart);
        if (nStartLineBegin >= 0)
        {
            int32 nBlockStart = Content.Find(TEXT("\n"), ESearchCase::CaseSensitive, ESearchDir::FromStart, nStartLineBegin);
            if (nBlockStart >= 0)
            {
                int32 nBlockEnd = Content.Find(TEXT("local"), ESearchCase::CaseSensitive, ESearchDir::FromStart, nBlockStart);
                Content.RemoveAt(nBlockStart, nBlockEnd - nBlockStart - 1);
                Content.InsertAt(nBlockStart + 1, DeclareString);
            }
            return true;
        }
    }
    return false;
}

// create Lua template file for the selected blueprint
bool CreateLuaTemplateFile(UBlueprint *Blueprint)
{
    if (Blueprint)
    {
        UClass *Class = Blueprint->GeneratedClass;
        UStruct* SuperStruct = Class->GetSuperStruct();
        bool bIsParentBP = (bool)Cast<UBlueprintGeneratedClass>(SuperStruct);
        FString ClassName = Class->GetName();
        FString ParentClassName = FString::Printf(TEXT("%s%s"), bIsParentBP ? TEXT("") : SuperStruct->GetPrefixCPP(), *SuperStruct->GetName());
        FString PropertyDeclare = GetEmmyLuaDeclareString(Class);
        FString FileName;
        UFunction* Func = Class->FindFunctionByName(FName("GetModuleName"));    // find UFunction 'GetModuleName'. hard coded!!!
        if (Func)
        {
            FString ModuleName;
            UObject* DefaultObject = Class->GetDefaultObject();             // get CDO
            DefaultObject->UObject::ProcessEvent(Func, &ModuleName);        // force to invoke UObject::ProcessEvent(...)
            if (!ModuleName.IsEmpty())
            {
                ModuleName = ModuleName.Replace(TEXT("."), TEXT("/"));
                FileName = FString::Printf(TEXT("%s%s.lua"), *GLuaSrcFullPath, *ModuleName);
            }
        }
        if (FileName.IsEmpty())
        {
            FString OuterPath = Class->GetPathName();
            int32 LastIndex;
            if (OuterPath.FindLastChar('/', LastIndex))
            {
                OuterPath = OuterPath.Left(LastIndex + 1);
            }
            OuterPath = OuterPath.RightChop(6);         // ignore "/Game/"
            FileName = FString::Printf(TEXT("%s%s%s.lua"), *GLuaSrcFullPath, *OuterPath, *ClassName);
        }
        if (FPaths::FileExists(FileName))
        {
            FString Content;
            FFileHelper::LoadFileToString(Content, *FileName);
            if (UpdatePropertyDeclare(Content, PropertyDeclare))
            {
                FFileHelper::SaveStringToFile(Content, *FileName, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
                //UE_LOG(LogUnLua, Warning, TEXT("Lua file (%s) is already existed!"), *ClassName);
                UE_LOG(LogUnLua, Log, TEXT("Lua file (%s) is successful updated."), *ClassName);
                return true;
            }
            return false;
        }

        static FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("UnLua"))->GetContentDir();

        FString TemplateName;
        if (Class->IsChildOf(AActor::StaticClass()))
        {
            // default BlueprintEvents for Actor
            TemplateName = ContentDir + TEXT("/ActorTemplate.lua");
        }
        else if (Class->IsChildOf(UUserWidget::StaticClass()))
        {
            // default BlueprintEvents for UserWidget (UMG)
            TemplateName = ContentDir + TEXT("/UserWidgetTemplate.lua");
        }
        else if (Class->IsChildOf(UAnimInstance::StaticClass()))
        {
            // default BlueprintEvents for AnimInstance (animation blueprint)
            TemplateName = ContentDir + TEXT("/AnimInstanceTemplate.lua");
        }
        else if (Class->IsChildOf(UActorComponent::StaticClass()))
        {
            // default BlueprintEvents for ActorComponent
            TemplateName = ContentDir + TEXT("/ActorComponentTemplate.lua");
        }

        FString Content;
        FFileHelper::LoadFileToString(Content, *TemplateName);
        Content = Content.Replace(TEXT("TemplateName"), *ClassName);
        Content = Content.Replace(TEXT("ParentClassName"), *ParentClassName);
        UpdatePropertyDeclare(Content, PropertyDeclare);

        return FFileHelper::SaveStringToFile(Content, *FileName, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    }
    return false;
}

