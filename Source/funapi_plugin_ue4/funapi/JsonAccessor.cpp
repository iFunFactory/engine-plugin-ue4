// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin_ue4.h"
#include "JsonAccessor.h"
#include "Json.h"

#include <assert.h>


namespace Fun
{
    // Default json accessor
    FString JsonAccessor<FJsonObject>::Serialize (TSharedPtr<FJsonObject> json_obj)
    {
        FString string;
        TSharedRef<TJsonWriter<>> writer = TJsonWriterFactory<>::Create(&string);
        if (FJsonSerializer::Serialize(json_obj.ToSharedRef(), writer))
            return string;

        return "";
    }

    TSharedPtr<FJsonObject> JsonAccessor<FJsonObject>::Deserialize (const FString& json_string)
    {
        TSharedPtr<FJsonObject> json = MakeShareable(new FJsonObject());
        TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(json_string);
        assert(reader != NULL);
        if (FJsonSerializer::Deserialize(reader, json) && json.IsValid())
            return json;

        return NULL;
    }

    FString JsonAccessor<FJsonObject>::GetStringField (TSharedPtr<FJsonObject> json_obj, const FString& field_name)
    {
        assert(json_obj != NULL);
        return json_obj->GetStringField(field_name);
    }

    void JsonAccessor<FJsonObject>::SetStringField (TSharedPtr<FJsonObject> json_obj, const FString& field_name, const FString& value)
    {
        assert(json_obj != NULL);
        json_obj->SetStringField(field_name, value);
    }

    int64_t JsonAccessor<FJsonObject>::GetIntegerField (TSharedPtr<FJsonObject> json_obj, const FString& field_name)
    {
        assert(json_obj != NULL);
        return json_obj->GetNumberField(field_name);
    }

    void JsonAccessor<FJsonObject>::SetIntegerField (TSharedPtr<FJsonObject> json_obj, const FString& field_name, int64_t value)
    {
        assert(json_obj != NULL);
        json_obj->SetNumberField(field_name, value);
    }

    bool JsonAccessor<FJsonObject>::HasField (TSharedPtr<FJsonObject> json_obj, const FString& field_name)
    {
        assert(json_obj != NULL);
        return json_obj->HasField(field_name);
    }

    void JsonAccessor<FJsonObject>::RemoveField (TSharedPtr<FJsonObject> json_obj, const FString& field_name)
    {
        assert(json_obj != NULL);
        json_obj->RemoveField(field_name);
    }

    TSharedPtr<FJsonObject> JsonAccessor<FJsonObject>::Clone (TSharedPtr<FJsonObject> json_obj)
    {
        assert(json_obj != NULL);
        return MakeShareable(new FJsonObject(*json_obj.Get()));
    }

} // namespace Fun
