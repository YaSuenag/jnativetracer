/*
 * Copyright (C) 2018 Yasumasa Suenaga
 *
 * This file is part of jnativetracer.
 *
 * jnativetracer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * jnativetracer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with jnativetracer.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <jvmti.h>
#include <jni.h>

#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <mutex>


std::recursive_mutex mtx;
thread_local bool already_owned = false;

jclass thread_class = NULL;
jmethodID dump_stack = NULL;
bool is_dump_stack = false;
char *target_class_name = NULL;
char *target_field_name = NULL;


static bool SetupTrigger(jvmtiEnv *jvmti, JNIEnv *jni){
  jclass target_class = jni->FindClass(target_class_name);
  if(target_class == NULL){

    if(jni->ExceptionCheck()){
      jni->ExceptionDescribe();
    }
    else{
      std::cerr << "Could not find " << target_class_name << std::endl;
    }

    return false;
  }

  jfieldID target_field = jni->GetFieldID(target_class, target_field_name, "Z");
  if(target_field == NULL){
    jni->ExceptionClear();
    target_field = jni->GetStaticFieldID(target_class, target_field_name, "Z");
  }
  if(target_field == NULL){

    if(jni->ExceptionCheck()){
      jni->ExceptionDescribe();
    }
    else{
      std::cerr << "Could not find " << target_field_name <<
                               " from " << target_class_name << std::endl;
    }

    return false;
  }


  jvmti->SetFieldModificationWatch(target_class, target_field);
  jvmti->SetEventNotificationMode(JVMTI_ENABLE,
                                    JVMTI_EVENT_FIELD_MODIFICATION, NULL);

  return true;
}

void JNICALL OnVMInit(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread){
  thread_class = jni->FindClass("java/lang/Thread");
  dump_stack = jni->GetStaticMethodID(thread_class, "dumpStack", "()V");

  if(target_class_name != NULL){
    if(SetupTrigger(jvmti, jni)){
      jvmti->SetEventNotificationMode(JVMTI_DISABLE,
                                        JVMTI_EVENT_METHOD_ENTRY, NULL);
      return;
    }
    else{
      std::cerr << "Could not set method trigger." <<
                          " Fallback to all-dump mode." << std::endl;
    }
  }

  jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, NULL);
}

void JNICALL OnMethodEntry(jvmtiEnv *jvmti, JNIEnv *jni,
                                 jthread thread, jmethodID method){
  jboolean is_native = JNI_FALSE;
  jvmti->IsMethodNative(method, &is_native);

  if(is_native){
    char *name = NULL;
    char *sig = NULL;
    jvmti->GetMethodName(method, &name, &sig, NULL);

    {
      std::lock_guard<std::recursive_mutex> lock(mtx);

      if(!already_owned){
        already_owned = true;
        std::cout << "jnativetracer: native method called: "
                                         << name << sig << std::endl;

        if(is_dump_stack){
          jni->CallStaticVoidMethod(thread_class, dump_stack);
        }

        std::cout << std::endl;
        already_owned = false;
      }

    }

    jvmti->Deallocate((unsigned char *)name);
    jvmti->Deallocate((unsigned char *)sig);
  }

}

void JNICALL OnFieldModification(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread,
                                 jmethodID method, jlocation location,
                                 jclass field_klass, jobject object,
                                 jfieldID field, char signature_type,
                                 jvalue new_value){
  jvmtiEventMode mode = new_value.z ? JVMTI_ENABLE : JVMTI_DISABLE;
  jvmti->SetEventNotificationMode(mode, JVMTI_EVENT_METHOD_ENTRY, NULL);
}

static bool extract_keyvalue(char *str, const char *delimiter,
                             const char *err_prefix, char **key, char **value){
  size_t delim_len = strlen(delimiter);

  *key = str;
  *value = strstr(*key, delimiter);

  if(*value == NULL){
    std::cerr << "Invalid configuration";
    if(err_prefix != NULL){
      std::cerr << ": " << err_prefix;
    }
    std::cerr << std::endl;
    return false;
  }

  char *ptr = *value;
  for(; ptr < (*value + delim_len); ptr++){
    *ptr = '\0';
  }
  *value = ptr;

  return true;
}

static bool parse_options(char *option_str){
  char *tok, *saveptr;
  char *options = option_str;

  while((tok = strtok_r(options, ",", &saveptr)) != NULL){
    options = NULL; // options must be NULL in subsequent calls.

    char *key, *value;
    if(!extract_keyvalue(tok, "=", NULL, &key, &value)){
      return false;
    }

    if(strcmp(key, "dumpstack") == 0){
      if(strcmp(value, "true") == 0){
        is_dump_stack = true;
      }
      else if(strcmp(value, "false") != 0){
        std::cerr << "Unknown dumpstack value: " << value << std::endl;
        return false;
      }
    }
    else if(strcmp(key, "trigger") == 0){

      if(!extract_keyvalue(value, "::", "trigger", &key, &value)){
        return false;
      }
      if(*value == '\0'){
        std::cerr << "Invalid trigger configuration" << std::endl;
        return false;
      }

      target_class_name = strdup(key);
      target_field_name = strdup(value);
    }
    else{
      std::cerr << "Unknown configuration: " << key << std::endl;
      return false;
    }

  }

  return true;
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved){

  if(options != NULL){
    if(!parse_options(options)){
      return JNI_ERR;
    }
  }

  jvmtiEnv *jvmti;
  vm->GetEnv((void **)&jvmti, JVMTI_VERSION_1);

  jvmtiEventCallbacks callbacks = {0};
  callbacks.VMInit = &OnVMInit;
  callbacks.FieldModification = &OnFieldModification;
  callbacks.MethodEntry = &OnMethodEntry;
  jvmti->SetEventCallbacks(&callbacks, sizeof(jvmtiEventCallbacks));

  jvmtiCapabilities capabilities = {0};
  capabilities.can_generate_field_modification_events = 1;
  capabilities.can_generate_method_entry_events = 1;
  jvmti->AddCapabilities(&capabilities);

  jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, NULL);

  return JNI_OK;
}

JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm){
  if(target_class_name) free(target_class_name);
  if(target_field_name) free(target_field_name);
}
