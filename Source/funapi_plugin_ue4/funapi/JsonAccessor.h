// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#pragma once

namespace Fun
{
    // Container to hold json-related functions.
    template <typename T>
    class JsonAccessor
    {
    public:
        virtual FString Serialize(TSharedPtr<T> json_obj) = 0;
        virtual TSharedPtr<T> Deserialize(const FString& json_string) = 0;
        virtual FString GetStringField(TSharedPtr<T> json_obj, const FString& field_name) = 0;
        virtual void SetStringField(TSharedPtr<T> json_obj, const FString& field_name, const FString& value) = 0;
        virtual int64_t GetIntegerField(TSharedPtr<T> json_obj, const FString& field_name) = 0;
        virtual void SetIntegerField(TSharedPtr<T> json_obj, const FString& field_name, int64_t value) = 0;
        virtual bool HasField(TSharedPtr<T> json_obj, const FString& field_name) = 0;
        virtual void RemoveField(TSharedPtr<T> json_obj, const FString& field_name) = 0;
        virtual TSharedPtr<T> Clone(TSharedPtr<T> json_obj) = 0;
    };


    // Default json accessor
    template <>
    class JsonAccessor<FJsonObject>
    {
    public:
        virtual FString Serialize(TSharedPtr<FJsonObject> json_obj);
        virtual TSharedPtr<FJsonObject> Deserialize(const FString& json_string);
        virtual FString GetStringField(TSharedPtr<FJsonObject> json_obj, const FString& field_name);
        virtual void SetStringField(TSharedPtr<FJsonObject> json_obj, const FString& field_name, const FString& value);
        virtual int64_t GetIntegerField(TSharedPtr<FJsonObject> json_obj, const FString& field_name);
        virtual void SetIntegerField(TSharedPtr<FJsonObject> json_obj, const FString& field_name, int64_t value);
        virtual bool HasField(TSharedPtr<FJsonObject> json_obj, const FString& field_name);
        virtual void RemoveField(TSharedPtr<FJsonObject> json_obj, const FString& field_name);
        virtual TSharedPtr<FJsonObject> Clone(TSharedPtr<FJsonObject> json_obj);
    };

} // namespace Fun
