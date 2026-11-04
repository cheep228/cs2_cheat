📦
148139 /src/menu.js
83133 /src/menu.js.map
✄
// node_modules/frida-il2cpp-bridge/dist/index.js
var __decorate = function(decorators, target, key, desc) {
  var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
  if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
  else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
  return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var Il2Cpp2;
(function(Il2Cpp3) {
  Il2Cpp3.application = {
    /**
     * Gets the data path name of the current application, e.g.
     * `/data/emulated/0/Android/data/com.example.application/files`
     * on Android.
     *
     * **This information is not guaranteed to exist.**
     *
     * ```ts
     * Il2Cpp.perform(() => {
     *     // prints /data/emulated/0/Android/data/com.example.application/files
     *     console.log(Il2Cpp.application.dataPath);
     * });
     * ```
     */
    get dataPath() {
      return unityEngineCall("get_persistentDataPath");
    },
    /**
     * Gets the identifier name of the current application, e.g.
     * `com.example.application` on Android.
     *
     * In case the identifier cannot be retrieved, the main module name is
     * returned instead, which typically is the process name.
     *
     * ```ts
     * Il2Cpp.perform(() => {
     *     // prints com.example.application
     *     console.log(Il2Cpp.application.identifier);
     * });
     * ```
     */
    get identifier() {
      return unityEngineCall("get_identifier") ?? unityEngineCall("get_bundleIdentifier") ?? Process.mainModule.name;
    },
    /**
     * Gets the version name of the current application, e.g. `4.12.8`.
     *
     * In case the version cannot be retrieved, an hash of the IL2CPP
     * module is returned instead.
     *
     * ```ts
     * Il2Cpp.perform(() => {
     *     // prints 4.12.8
     *     console.log(Il2Cpp.application.version);
     * });
     * ```
     */
    get version() {
      return unityEngineCall("get_version") ?? Il2Cpp3.module.enumerateSections().reduce((checksum, section) => {
        return [".text", "__text"].includes(section.name) ? checksum.update(ArrayBuffer.wrap(section.address, section.size)) : checksum;
      }, new Checksum("md5")).getString();
    }
  };
  getter(Il2Cpp3, "unityVersion", () => {
    try {
      const unityVersion = Il2Cpp3.$config.unityVersion ?? unityEngineCall("get_unityVersion");
      if (unityVersion != null) {
        return unityVersion;
      }
    } catch (_) {
    }
    const rangeProvider = function* () {
      yield* Il2Cpp3.module.enumerateSections().filter((_) => _.name == ".rodata" || _.name == ".rdata" || _.name == "__TEXT").map((_) => ({ base: _.address, size: _.size }));
      yield* Il2Cpp3.module.enumerateRanges("r--");
      yield* Process.enumerateRanges("r--").filter((_) => _.file != void 0 && _.file.path != Il2Cpp3.module.path);
    };
    for (const range of rangeProvider()) {
      let matches;
      try {
        matches = Memory.scanSync(range.base, range.size, "69 6c 32 63 70 70");
      } catch (_) {
        continue;
      }
      for (let { address } of matches) {
        while (address.readU8() != 0) {
          address = address.sub(1);
        }
        const match = UnityVersion.find(address.add(1).readCString());
        if (match != void 0) {
          return match;
        }
      }
    }
    raise("couldn't determine the Unity version, please specify it manually");
  }, lazy);
  function unityEngineCall(method) {
    const icallHandle = Il2Cpp3.exports.resolveInternalCall(Memory.allocUtf8String("UnityEngine.Application::" + method));
    return (icallHandle.isNull() ? Il2Cpp3.domain.tryAssembly("UnityEngine.CoreModule")?.image?.tryClass("UnityEngine.Application")?.tryMethod(method)?.invoke() : new Il2Cpp3.String(new NativeFunction(icallHandle, "pointer", [])()))?.asNullable()?.content ?? null;
  }
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  function boxed(value, type) {
    const mapping = {
      int8: "System.SByte",
      uint8: "System.Byte",
      int16: "System.Int16",
      uint16: "System.UInt16",
      int32: "System.Int32",
      uint32: "System.UInt32",
      int64: "System.Int64",
      uint64: "System.UInt64",
      char: "System.Char",
      intptr: "System.IntPtr",
      uintptr: "System.UIntPtr"
    };
    const className = typeof value == "boolean" ? "System.Boolean" : typeof value == "number" ? mapping[type ?? "int32"] : value instanceof Int64 ? "System.Int64" : value instanceof UInt64 ? "System.UInt64" : value instanceof NativePointer ? mapping[type ?? "intptr"] : raise(`Cannot create boxed primitive using value of type '${typeof value}'`);
    const object = Il2Cpp3.corlib.class(className ?? raise(`Unknown primitive type name '${type}'`)).alloc();
    (object.tryField("m_value") ?? object.tryField("_pointer") ?? raise(`Could not find primitive field in class '${className}'`)).value = value;
    return object;
  }
  Il2Cpp3.boxed = boxed;
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  Il2Cpp3.$config = {
    moduleName: void 0,
    unityVersion: void 0,
    exports: void 0
  };
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  function installExceptionListener(targetThread = "current") {
    const currentThread = Il2Cpp3.exports.threadGetCurrent();
    return Interceptor.attach(Il2Cpp3.module.getExportByName("__cxa_throw"), function(args) {
      if (targetThread == "current" && !Il2Cpp3.exports.threadGetCurrent().equals(currentThread)) {
        return;
      }
      inform(new Il2Cpp3.Object(args[0].readPointer()));
    });
  }
  Il2Cpp3.installExceptionListener = installExceptionListener;
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  Il2Cpp3.exports = {
    get alloc() {
      return r("il2cpp_alloc", "pointer", ["size_t"]);
    },
    get arrayGetLength() {
      return r("il2cpp_array_length", "uint32", ["pointer"]);
    },
    get arrayNew() {
      return r("il2cpp_array_new", "pointer", ["pointer", "uint32"]);
    },
    get assemblyGetImage() {
      return r("il2cpp_assembly_get_image", "pointer", ["pointer"]);
    },
    get classForEach() {
      return r("il2cpp_class_for_each", "void", ["pointer", "pointer"]);
    },
    get classFromName() {
      return r("il2cpp_class_from_name", "pointer", ["pointer", "pointer", "pointer"]);
    },
    get classFromObject() {
      return r("il2cpp_class_from_system_type", "pointer", ["pointer"]);
    },
    get classGetArrayClass() {
      return r("il2cpp_array_class_get", "pointer", ["pointer", "uint32"]);
    },
    get classGetArrayElementSize() {
      return r("il2cpp_class_array_element_size", "int", ["pointer"]);
    },
    get classGetAssemblyName() {
      return r("il2cpp_class_get_assemblyname", "pointer", ["pointer"]);
    },
    get classGetBaseType() {
      return r("il2cpp_class_enum_basetype", "pointer", ["pointer"]);
    },
    get classGetDeclaringType() {
      return r("il2cpp_class_get_declaring_type", "pointer", ["pointer"]);
    },
    get classGetElementClass() {
      return r("il2cpp_class_get_element_class", "pointer", ["pointer"]);
    },
    get classGetFieldFromName() {
      return r("il2cpp_class_get_field_from_name", "pointer", ["pointer", "pointer"]);
    },
    get classGetFields() {
      return r("il2cpp_class_get_fields", "pointer", ["pointer", "pointer"]);
    },
    get classGetFlags() {
      return r("il2cpp_class_get_flags", "int", ["pointer"]);
    },
    get classGetImage() {
      return r("il2cpp_class_get_image", "pointer", ["pointer"]);
    },
    get classGetInstanceSize() {
      return r("il2cpp_class_instance_size", "int32", ["pointer"]);
    },
    get classGetInterfaces() {
      return r("il2cpp_class_get_interfaces", "pointer", ["pointer", "pointer"]);
    },
    get classGetMethodFromName() {
      return r("il2cpp_class_get_method_from_name", "pointer", ["pointer", "pointer", "int"]);
    },
    get classGetMethods() {
      return r("il2cpp_class_get_methods", "pointer", ["pointer", "pointer"]);
    },
    get classGetName() {
      return r("il2cpp_class_get_name", "pointer", ["pointer"]);
    },
    get classGetNamespace() {
      return r("il2cpp_class_get_namespace", "pointer", ["pointer"]);
    },
    get classGetNestedClasses() {
      return r("il2cpp_class_get_nested_types", "pointer", ["pointer", "pointer"]);
    },
    get classGetParent() {
      return r("il2cpp_class_get_parent", "pointer", ["pointer"]);
    },
    get classGetStaticFieldData() {
      return r("il2cpp_class_get_static_field_data", "pointer", ["pointer"]);
    },
    get classGetValueTypeSize() {
      return r("il2cpp_class_value_size", "int32", ["pointer", "pointer"]);
    },
    get classGetType() {
      return r("il2cpp_class_get_type", "pointer", ["pointer"]);
    },
    get classHasReferences() {
      return r("il2cpp_class_has_references", "bool", ["pointer"]);
    },
    get classInitialize() {
      return r("il2cpp_runtime_class_init", "void", ["pointer"]);
    },
    get classIsAbstract() {
      return r("il2cpp_class_is_abstract", "bool", ["pointer"]);
    },
    get classIsAssignableFrom() {
      return r("il2cpp_class_is_assignable_from", "bool", ["pointer", "pointer"]);
    },
    get classIsBlittable() {
      return r("il2cpp_class_is_blittable", "bool", ["pointer"]);
    },
    get classIsEnum() {
      return r("il2cpp_class_is_enum", "bool", ["pointer"]);
    },
    get classIsGeneric() {
      return r("il2cpp_class_is_generic", "bool", ["pointer"]);
    },
    get classIsInflated() {
      return r("il2cpp_class_is_inflated", "bool", ["pointer"]);
    },
    get classIsInterface() {
      return r("il2cpp_class_is_interface", "bool", ["pointer"]);
    },
    get classIsSubclassOf() {
      return r("il2cpp_class_is_subclass_of", "bool", ["pointer", "pointer", "bool"]);
    },
    get classIsValueType() {
      return r("il2cpp_class_is_valuetype", "bool", ["pointer"]);
    },
    get domainGetAssemblyFromName() {
      return r("il2cpp_domain_assembly_open", "pointer", ["pointer", "pointer"]);
    },
    get domainGet() {
      return r("il2cpp_domain_get", "pointer", []);
    },
    get domainGetAssemblies() {
      return r("il2cpp_domain_get_assemblies", "pointer", ["pointer", "pointer"]);
    },
    get fieldGetClass() {
      return r("il2cpp_field_get_parent", "pointer", ["pointer"]);
    },
    get fieldGetFlags() {
      return r("il2cpp_field_get_flags", "int", ["pointer"]);
    },
    get fieldGetName() {
      return r("il2cpp_field_get_name", "pointer", ["pointer"]);
    },
    get fieldGetOffset() {
      return r("il2cpp_field_get_offset", "int32", ["pointer"]);
    },
    get fieldGetStaticValue() {
      return r("il2cpp_field_static_get_value", "void", ["pointer", "pointer"]);
    },
    get fieldGetType() {
      return r("il2cpp_field_get_type", "pointer", ["pointer"]);
    },
    get fieldSetStaticValue() {
      return r("il2cpp_field_static_set_value", "void", ["pointer", "pointer"]);
    },
    get free() {
      return r("il2cpp_free", "void", ["pointer"]);
    },
    get gcCollect() {
      return r("il2cpp_gc_collect", "void", ["int"]);
    },
    get gcCollectALittle() {
      return r("il2cpp_gc_collect_a_little", "void", []);
    },
    get gcDisable() {
      return r("il2cpp_gc_disable", "void", []);
    },
    get gcEnable() {
      return r("il2cpp_gc_enable", "void", []);
    },
    get gcForEachHeap() {
      return r("il2cpp_gc_foreach_heap", "void", ["pointer", "pointer"]);
    },
    get gcGetHeapSize() {
      return r("il2cpp_gc_get_heap_size", "int64", []);
    },
    get gcGetMaxTimeSlice() {
      return r("il2cpp_gc_get_max_time_slice_ns", "int64", []);
    },
    get gcGetUsedSize() {
      return r("il2cpp_gc_get_used_size", "int64", []);
    },
    get gcHandleGetTarget() {
      return r("il2cpp_gchandle_get_target", "pointer", ["uint32"]);
    },
    get gcHandleFree() {
      return r("il2cpp_gchandle_free", "void", ["uint32"]);
    },
    get gcHandleNew() {
      return r("il2cpp_gchandle_new", "uint32", ["pointer", "bool"]);
    },
    get gcHandleNewWeakRef() {
      return r("il2cpp_gchandle_new_weakref", "uint32", ["pointer", "bool"]);
    },
    get gcIsDisabled() {
      return r("il2cpp_gc_is_disabled", "bool", []);
    },
    get gcIsIncremental() {
      return r("il2cpp_gc_is_incremental", "bool", []);
    },
    get gcSetMaxTimeSlice() {
      return r("il2cpp_gc_set_max_time_slice_ns", "void", ["int64"]);
    },
    get gcStartIncrementalCollection() {
      return r("il2cpp_gc_start_incremental_collection", "void", []);
    },
    get gcStartWorld() {
      return r("il2cpp_start_gc_world", "void", []);
    },
    get gcStopWorld() {
      return r("il2cpp_stop_gc_world", "void", []);
    },
    get getCorlib() {
      return r("il2cpp_get_corlib", "pointer", []);
    },
    get imageGetAssembly() {
      return r("il2cpp_image_get_assembly", "pointer", ["pointer"]);
    },
    get imageGetClass() {
      return r("il2cpp_image_get_class", "pointer", ["pointer", "uint"]);
    },
    get imageGetClassCount() {
      return r("il2cpp_image_get_class_count", "uint32", ["pointer"]);
    },
    get imageGetName() {
      return r("il2cpp_image_get_name", "pointer", ["pointer"]);
    },
    get initialize() {
      return r("il2cpp_init", "void", ["pointer"]);
    },
    get livenessAllocateStruct() {
      return r("il2cpp_unity_liveness_allocate_struct", "pointer", ["pointer", "int", "pointer", "pointer", "pointer"]);
    },
    get livenessCalculationBegin() {
      return r("il2cpp_unity_liveness_calculation_begin", "pointer", ["pointer", "int", "pointer", "pointer", "pointer", "pointer"]);
    },
    get livenessCalculationEnd() {
      return r("il2cpp_unity_liveness_calculation_end", "void", ["pointer"]);
    },
    get livenessCalculationFromStatics() {
      return r("il2cpp_unity_liveness_calculation_from_statics", "void", ["pointer"]);
    },
    get livenessFinalize() {
      return r("il2cpp_unity_liveness_finalize", "void", ["pointer"]);
    },
    get livenessFreeStruct() {
      return r("il2cpp_unity_liveness_free_struct", "void", ["pointer"]);
    },
    get memorySnapshotCapture() {
      return r("il2cpp_capture_memory_snapshot", "pointer", []);
    },
    get memorySnapshotFree() {
      return r("il2cpp_free_captured_memory_snapshot", "void", ["pointer"]);
    },
    get memorySnapshotGetClasses() {
      return r("il2cpp_memory_snapshot_get_classes", "pointer", ["pointer", "pointer"]);
    },
    get memorySnapshotGetObjects() {
      return r("il2cpp_memory_snapshot_get_objects", "pointer", ["pointer", "pointer"]);
    },
    get methodGetClass() {
      return r("il2cpp_method_get_class", "pointer", ["pointer"]);
    },
    get methodGetFlags() {
      return r("il2cpp_method_get_flags", "uint32", ["pointer", "pointer"]);
    },
    get methodGetName() {
      return r("il2cpp_method_get_name", "pointer", ["pointer"]);
    },
    get methodGetObject() {
      return r("il2cpp_method_get_object", "pointer", ["pointer", "pointer"]);
    },
    get methodGetParameterCount() {
      return r("il2cpp_method_get_param_count", "uint8", ["pointer"]);
    },
    get methodGetParameterName() {
      return r("il2cpp_method_get_param_name", "pointer", ["pointer", "uint32"]);
    },
    get methodGetParameters() {
      return r("il2cpp_method_get_parameters", "pointer", ["pointer", "pointer"]);
    },
    get methodGetParameterType() {
      return r("il2cpp_method_get_param", "pointer", ["pointer", "uint32"]);
    },
    get methodGetReturnType() {
      return r("il2cpp_method_get_return_type", "pointer", ["pointer"]);
    },
    get methodIsGeneric() {
      return r("il2cpp_method_is_generic", "bool", ["pointer"]);
    },
    get methodIsInflated() {
      return r("il2cpp_method_is_inflated", "bool", ["pointer"]);
    },
    get methodIsInstance() {
      return r("il2cpp_method_is_instance", "bool", ["pointer"]);
    },
    get monitorEnter() {
      return r("il2cpp_monitor_enter", "void", ["pointer"]);
    },
    get monitorExit() {
      return r("il2cpp_monitor_exit", "void", ["pointer"]);
    },
    get monitorPulse() {
      return r("il2cpp_monitor_pulse", "void", ["pointer"]);
    },
    get monitorPulseAll() {
      return r("il2cpp_monitor_pulse_all", "void", ["pointer"]);
    },
    get monitorTryEnter() {
      return r("il2cpp_monitor_try_enter", "bool", ["pointer", "uint32"]);
    },
    get monitorTryWait() {
      return r("il2cpp_monitor_try_wait", "bool", ["pointer", "uint32"]);
    },
    get monitorWait() {
      return r("il2cpp_monitor_wait", "void", ["pointer"]);
    },
    get objectGetClass() {
      return r("il2cpp_object_get_class", "pointer", ["pointer"]);
    },
    get objectGetVirtualMethod() {
      return r("il2cpp_object_get_virtual_method", "pointer", ["pointer", "pointer"]);
    },
    get objectInitialize() {
      return r("il2cpp_runtime_object_init_exception", "void", ["pointer", "pointer"]);
    },
    get objectNew() {
      return r("il2cpp_object_new", "pointer", ["pointer"]);
    },
    get objectGetSize() {
      return r("il2cpp_object_get_size", "uint32", ["pointer"]);
    },
    get objectUnbox() {
      return r("il2cpp_object_unbox", "pointer", ["pointer"]);
    },
    get resolveInternalCall() {
      return r("il2cpp_resolve_icall", "pointer", ["pointer"]);
    },
    get stringGetChars() {
      return r("il2cpp_string_chars", "pointer", ["pointer"]);
    },
    get stringGetLength() {
      return r("il2cpp_string_length", "int32", ["pointer"]);
    },
    get stringNew() {
      return r("il2cpp_string_new", "pointer", ["pointer"]);
    },
    get valueTypeBox() {
      return r("il2cpp_value_box", "pointer", ["pointer", "pointer"]);
    },
    get threadAttach() {
      return r("il2cpp_thread_attach", "pointer", ["pointer"]);
    },
    get threadDetach() {
      return r("il2cpp_thread_detach", "void", ["pointer"]);
    },
    get threadGetAttachedThreads() {
      return r("il2cpp_thread_get_all_attached_threads", "pointer", ["pointer"]);
    },
    get threadGetCurrent() {
      return r("il2cpp_thread_current", "pointer", []);
    },
    get threadIsVm() {
      return r("il2cpp_is_vm_thread", "bool", ["pointer"]);
    },
    get typeEquals() {
      return r("il2cpp_type_equals", "bool", ["pointer", "pointer"]);
    },
    get typeGetClass() {
      return r("il2cpp_class_from_type", "pointer", ["pointer"]);
    },
    get typeGetName() {
      return r("il2cpp_type_get_name", "pointer", ["pointer"]);
    },
    get typeGetObject() {
      return r("il2cpp_type_get_object", "pointer", ["pointer"]);
    },
    get typeGetTypeEnum() {
      return r("il2cpp_type_get_type", "int", ["pointer"]);
    }
  };
  decorate(Il2Cpp3.exports, lazy);
  getter(Il2Cpp3, "memorySnapshotExports", () => new CModule("#include <stdint.h>\n#include <string.h>\n\ntypedef struct Il2CppManagedMemorySnapshot Il2CppManagedMemorySnapshot;\ntypedef struct Il2CppMetadataType Il2CppMetadataType;\n\nstruct Il2CppManagedMemorySnapshot\n{\n  struct Il2CppManagedHeap\n  {\n    uint32_t section_count;\n    void * sections;\n  } heap;\n  struct Il2CppStacks\n  {\n    uint32_t stack_count;\n    void * stacks;\n  } stacks;\n  struct Il2CppMetadataSnapshot\n  {\n    uint32_t type_count;\n    Il2CppMetadataType * types;\n  } metadata_snapshot;\n  struct Il2CppGCHandles\n  {\n    uint32_t tracked_object_count;\n    void ** pointers_to_objects;\n  } gc_handles;\n  struct Il2CppRuntimeInformation\n  {\n    uint32_t pointer_size;\n    uint32_t object_header_size;\n    uint32_t array_header_size;\n    uint32_t array_bounds_offset_in_header;\n    uint32_t array_size_offset_in_header;\n    uint32_t allocation_granularity;\n  } runtime_information;\n  void * additional_user_information;\n};\n\nstruct Il2CppMetadataType\n{\n  uint32_t flags;\n  void * fields;\n  uint32_t field_count;\n  uint32_t statics_size;\n  uint8_t * statics;\n  uint32_t base_or_element_type_index;\n  char * name;\n  const char * assembly_name;\n  uint64_t type_info_address;\n  uint32_t size;\n};\n\nuintptr_t\nil2cpp_memory_snapshot_get_classes (\n    const Il2CppManagedMemorySnapshot * snapshot, Il2CppMetadataType ** iter)\n{\n  const int zero = 0;\n  const void * null = 0;\n\n  if (iter != NULL && snapshot->metadata_snapshot.type_count > zero)\n  {\n    if (*iter == null)\n    {\n      *iter = snapshot->metadata_snapshot.types;\n      return (uintptr_t) (*iter)->type_info_address;\n    }\n    else\n    {\n      Il2CppMetadataType * metadata_type = *iter + 1;\n\n      if (metadata_type < snapshot->metadata_snapshot.types +\n                              snapshot->metadata_snapshot.type_count)\n      {\n        *iter = metadata_type;\n        return (uintptr_t) (*iter)->type_info_address;\n      }\n    }\n  }\n  return 0;\n}\n\nvoid **\nil2cpp_memory_snapshot_get_objects (\n    const Il2CppManagedMemorySnapshot * snapshot, uint32_t * size)\n{\n  *size = snapshot->gc_handles.tracked_object_count;\n  return snapshot->gc_handles.pointers_to_objects;\n}\n"), lazy);
  function r(exportName, retType, argTypes) {
    const handle = Il2Cpp3.$config.exports?.[exportName]?.() ?? Il2Cpp3.module.findExportByName(exportName) ?? Il2Cpp3.memorySnapshotExports[exportName];
    const target = new NativeFunction(handle ?? NULL, retType, argTypes);
    return target.isNull() ? new Proxy(target, {
      get(value, name) {
        const property = value[name];
        return typeof property === "function" ? property.bind(value) : property;
      },
      apply() {
        if (handle == null) {
          raise(`couldn't resolve export ${exportName}`);
        } else if (handle.isNull()) {
          raise(`export ${exportName} points to NULL IL2CPP library has likely been stripped, obfuscated, or customized`);
        }
      }
    }) : target;
  }
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  function is(klass) {
    return (element) => {
      if (element instanceof Il2Cpp3.Class) {
        return klass.isAssignableFrom(element);
      } else {
        return klass.isAssignableFrom(element.class);
      }
    };
  }
  Il2Cpp3.is = is;
  function isExactly(klass) {
    return (element) => {
      if (element instanceof Il2Cpp3.Class) {
        return element.equals(klass);
      } else {
        return element.class.equals(klass);
      }
    };
  }
  Il2Cpp3.isExactly = isExactly;
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  Il2Cpp3.gc = {
    /**
     * Gets the allocated heap sections managed by IL2CPP.
     */
    get heapSections() {
      const ranges = [];
      Il2Cpp3.exports.gcForEachHeap(new NativeCallback((data, _) => {
        ranges.push({
          base: data.readPointer(),
          size: data.add(Process.pointerSize).readU32()
        });
      }, "void", ["pointer", "pointer"]), NULL);
      return ranges;
    },
    /**
     * Gets the heap size in bytes.
     */
    get heapSize() {
      return Il2Cpp3.exports.gcGetHeapSize();
    },
    /**
     * Determines whether the garbage collector is enabled.
     */
    get isEnabled() {
      return !Il2Cpp3.exports.gcIsDisabled();
    },
    /**
     * Determines whether the garbage collector is incremental
     * ([source](https://docs.unity3d.com/Manual/performance-incremental-garbage-collection.html)).
     */
    get isIncremental() {
      return !!Il2Cpp3.exports.gcIsIncremental();
    },
    /**
     * Gets the number of nanoseconds the garbage collector can spend in a
     * collection step.
     */
    get maxTimeSlice() {
      return Il2Cpp3.exports.gcGetMaxTimeSlice();
    },
    /**
     * Gets the used heap size in bytes.
     */
    get usedHeapSize() {
      return Il2Cpp3.exports.gcGetUsedSize();
    },
    /**
     * Enables or disables the garbage collector.
     */
    set isEnabled(value) {
      value ? Il2Cpp3.exports.gcEnable() : Il2Cpp3.exports.gcDisable();
    },
    /**
     *  Sets the number of nanoseconds the garbage collector can spend in
     * a collection step.
     */
    set maxTimeSlice(nanoseconds) {
      Il2Cpp3.exports.gcSetMaxTimeSlice(nanoseconds);
    },
    /**
     * Returns the heap allocated objects of the specified class. \
     * This variant reads GC descriptors.
     */
    choose(klass) {
      const matches = [];
      const callback = (objects, size) => {
        for (let i = 0; i < size; i++) {
          matches.push(new Il2Cpp3.Object(objects.add(i * Process.pointerSize).readPointer()));
        }
      };
      const chooseCallback = new NativeCallback(callback, "void", ["pointer", "int", "pointer"]);
      if (!Il2Cpp3.exports.livenessCalculationBegin.isNull()) {
        const onWorld = new NativeCallback(() => {
        }, "void", []);
        const state = Il2Cpp3.exports.livenessCalculationBegin(klass, 0, chooseCallback, NULL, onWorld, onWorld);
        Il2Cpp3.exports.livenessCalculationFromStatics(state);
        Il2Cpp3.exports.livenessCalculationEnd(state);
      } else {
        const realloc = (handle, size) => {
          if (!handle.isNull() && size.compare(0) == 0) {
            Il2Cpp3.free(handle);
            return NULL;
          } else {
            return Il2Cpp3.alloc(size);
          }
        };
        const reallocCallback = new NativeCallback(realloc, "pointer", ["pointer", "size_t", "pointer"]);
        this.stopWorld();
        const state = Il2Cpp3.exports.livenessAllocateStruct(klass, 0, chooseCallback, NULL, reallocCallback);
        Il2Cpp3.exports.livenessCalculationFromStatics(state);
        Il2Cpp3.exports.livenessFinalize(state);
        this.startWorld();
        Il2Cpp3.exports.livenessFreeStruct(state);
      }
      return matches;
    },
    /**
     * Forces a garbage collection of the specified generation.
     */
    collect(generation) {
      Il2Cpp3.exports.gcCollect(generation < 0 ? 0 : generation > 2 ? 2 : generation);
    },
    /**
     * Forces a garbage collection.
     */
    collectALittle() {
      Il2Cpp3.exports.gcCollectALittle();
    },
    /**
     *  Resumes all the previously stopped threads.
     */
    startWorld() {
      return Il2Cpp3.exports.gcStartWorld();
    },
    /**
     * Performs an incremental garbage collection.
     */
    startIncrementalCollection() {
      return Il2Cpp3.exports.gcStartIncrementalCollection();
    },
    /**
     * Stops all threads which may access the garbage collected heap, other
     * than the caller.
     */
    stopWorld() {
      return Il2Cpp3.exports.gcStopWorld();
    }
  };
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Android;
(function(Android2) {
  getter(Android2, "apiLevel", () => {
    const value = getProperty("ro.build.version.sdk");
    return value ? parseInt(value) : null;
  }, lazy);
  function getProperty(name) {
    const handle = Process.findModuleByName("libc.so")?.findExportByName("__system_property_get");
    if (handle) {
      const __system_property_get = new NativeFunction(handle, "void", ["pointer", "pointer"]);
      const value = Memory.alloc(92).writePointer(NULL);
      __system_property_get(Memory.allocUtf8String(name), value);
      return value.readCString() ?? void 0;
    }
  }
})(Android || (Android = {}));
function raise(message) {
  const error = new Error(message);
  error.name = "Il2CppError";
  error.stack = error.stack?.replace(/^(Il2Cpp)?Error/, "\x1B[0m\x1B[38;5;9mil2cpp\x1B[0m")?.replace(/\n    at (.+) \((.+):(.+)\)/, "\x1B[3m\x1B[2m")?.concat("\x1B[0m");
  throw error;
}
function warn(message) {
  globalThis.console.log(`\x1B[38;5;11mil2cpp\x1B[0m: ${message}`);
}
function inform(message) {
  globalThis.console.log(`\x1B[38;5;12mil2cpp\x1B[0m: ${message}`);
}
function decorate(target, decorator, descriptors = Object.getOwnPropertyDescriptors(target)) {
  for (const key in descriptors) {
    descriptors[key] = decorator(target, key, descriptors[key]);
  }
  Object.defineProperties(target, descriptors);
  return target;
}
function getter(target, key, get, decorator) {
  globalThis.Object.defineProperty(target, key, decorator?.(target, key, { get, configurable: true }) ?? { get, configurable: true });
}
function lazy(_, propertyKey, descriptor) {
  const getter2 = descriptor.get;
  if (!getter2) {
    throw new Error("@lazy can only be applied to getter accessors");
  }
  descriptor.get = function() {
    const value = getter2.call(this);
    Object.defineProperty(this, propertyKey, {
      value,
      configurable: descriptor.configurable,
      enumerable: descriptor.enumerable,
      writable: false
    });
    return value;
  };
  return descriptor;
}
var NativeStruct = class {
  handle;
  constructor(handleOrWrapper) {
    if (handleOrWrapper instanceof NativePointer) {
      this.handle = handleOrWrapper;
    } else {
      this.handle = handleOrWrapper.handle;
    }
  }
  equals(other) {
    return this.handle.equals(other.handle);
  }
  isNull() {
    return this.handle.isNull();
  }
  asNullable() {
    return this.isNull() ? null : this;
  }
};
function addFlippedEntries(obj) {
  return Object.keys(obj).reduce((obj2, key) => (obj2[obj2[key]] = key, obj2), obj);
}
NativePointer.prototype.offsetOf = function(condition, depth) {
  depth ??= 512;
  for (let i = 0; depth > 0 ? i < depth : i < -depth; i++) {
    if (condition(depth > 0 ? this.add(i) : this.sub(i))) {
      return i;
    }
  }
  return null;
};
function readNativeIterator(block) {
  const array = [];
  const iterator = Memory.alloc(Process.pointerSize);
  let handle = block(iterator);
  while (!handle.isNull()) {
    array.push(handle);
    handle = block(iterator);
  }
  return array;
}
function readNativeList(block) {
  const lengthPointer = Memory.alloc(Process.pointerSize);
  const startPointer = block(lengthPointer);
  if (startPointer.isNull()) {
    return [];
  }
  const array = new Array(lengthPointer.readInt());
  for (let i = 0; i < array.length; i++) {
    array[i] = startPointer.add(i * Process.pointerSize).readPointer();
  }
  return array;
}
function recycle(Class) {
  return new Proxy(Class, {
    cache: /* @__PURE__ */ new Map(),
    construct(Target, argArray) {
      const handle = argArray[0].toUInt32();
      if (!this.cache.has(handle)) {
        this.cache.set(handle, new Target(argArray[0]));
      }
      return this.cache.get(handle);
    }
  });
}
var UnityVersion;
(function(UnityVersion2) {
  const pattern = /(6\d{3}|20\d{2}|\d)\.(\d)\.(\d{1,2})(?:[abcfp]|rc){0,2}\d?/;
  function find(string) {
    return string?.match(pattern)?.[0];
  }
  UnityVersion2.find = find;
  function gte(a, b) {
    return compare(a, b) >= 0;
  }
  UnityVersion2.gte = gte;
  function lt(a, b) {
    return compare(a, b) < 0;
  }
  UnityVersion2.lt = lt;
  function compare(a, b) {
    const aMatches = a.match(pattern);
    const bMatches = b.match(pattern);
    for (let i = 1; i <= 3; i++) {
      const a2 = Number(aMatches?.[i] ?? -1);
      const b2 = Number(bMatches?.[i] ?? -1);
      if (a2 > b2)
        return 1;
      else if (a2 < b2)
        return -1;
    }
    return 0;
  }
})(UnityVersion || (UnityVersion = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  function alloc(size = Process.pointerSize) {
    return Il2Cpp3.exports.alloc(size);
  }
  Il2Cpp3.alloc = alloc;
  function free(pointer) {
    return Il2Cpp3.exports.free(pointer);
  }
  Il2Cpp3.free = free;
  function read(pointer, type) {
    switch (type.enumValue) {
      case Il2Cpp3.Type.Enum.BOOLEAN:
        return !!pointer.readS8();
      case Il2Cpp3.Type.Enum.BYTE:
        return pointer.readS8();
      case Il2Cpp3.Type.Enum.UBYTE:
        return pointer.readU8();
      case Il2Cpp3.Type.Enum.SHORT:
        return pointer.readS16();
      case Il2Cpp3.Type.Enum.USHORT:
        return pointer.readU16();
      case Il2Cpp3.Type.Enum.INT:
        return pointer.readS32();
      case Il2Cpp3.Type.Enum.UINT:
        return pointer.readU32();
      case Il2Cpp3.Type.Enum.CHAR:
        return pointer.readU16();
      case Il2Cpp3.Type.Enum.LONG:
        return pointer.readS64();
      case Il2Cpp3.Type.Enum.ULONG:
        return pointer.readU64();
      case Il2Cpp3.Type.Enum.FLOAT:
        return pointer.readFloat();
      case Il2Cpp3.Type.Enum.DOUBLE:
        return pointer.readDouble();
      case Il2Cpp3.Type.Enum.NINT:
      case Il2Cpp3.Type.Enum.NUINT:
        return pointer.readPointer();
      case Il2Cpp3.Type.Enum.POINTER:
        return new Il2Cpp3.Pointer(pointer.readPointer(), type.class.baseType);
      case Il2Cpp3.Type.Enum.VALUE_TYPE:
        return new Il2Cpp3.ValueType(pointer, type);
      case Il2Cpp3.Type.Enum.OBJECT:
      case Il2Cpp3.Type.Enum.CLASS:
        return new Il2Cpp3.Object(pointer.readPointer());
      case Il2Cpp3.Type.Enum.GENERIC_INSTANCE:
        return type.class.isValueType ? new Il2Cpp3.ValueType(pointer, type) : new Il2Cpp3.Object(pointer.readPointer());
      case Il2Cpp3.Type.Enum.STRING:
        return new Il2Cpp3.String(pointer.readPointer());
      case Il2Cpp3.Type.Enum.ARRAY:
      case Il2Cpp3.Type.Enum.NARRAY:
        return new Il2Cpp3.Array(pointer.readPointer());
    }
    raise(`couldn't read the value from ${pointer} using an unhandled or unknown type ${type.name} (${type.enumValue}), please file an issue`);
  }
  Il2Cpp3.read = read;
  function write(pointer, value, type) {
    switch (type.enumValue) {
      case Il2Cpp3.Type.Enum.BOOLEAN:
        return pointer.writeS8(+value);
      case Il2Cpp3.Type.Enum.BYTE:
        return pointer.writeS8(value);
      case Il2Cpp3.Type.Enum.UBYTE:
        return pointer.writeU8(value);
      case Il2Cpp3.Type.Enum.SHORT:
        return pointer.writeS16(value);
      case Il2Cpp3.Type.Enum.USHORT:
        return pointer.writeU16(value);
      case Il2Cpp3.Type.Enum.INT:
        return pointer.writeS32(value);
      case Il2Cpp3.Type.Enum.UINT:
        return pointer.writeU32(value);
      case Il2Cpp3.Type.Enum.CHAR:
        return pointer.writeU16(value);
      case Il2Cpp3.Type.Enum.LONG:
        return pointer.writeS64(value);
      case Il2Cpp3.Type.Enum.ULONG:
        return pointer.writeU64(value);
      case Il2Cpp3.Type.Enum.FLOAT:
        return pointer.writeFloat(value);
      case Il2Cpp3.Type.Enum.DOUBLE:
        return pointer.writeDouble(value);
      case Il2Cpp3.Type.Enum.NINT:
      case Il2Cpp3.Type.Enum.NUINT:
      case Il2Cpp3.Type.Enum.POINTER:
      case Il2Cpp3.Type.Enum.STRING:
      case Il2Cpp3.Type.Enum.ARRAY:
      case Il2Cpp3.Type.Enum.NARRAY:
        return pointer.writePointer(value);
      case Il2Cpp3.Type.Enum.VALUE_TYPE:
        return Memory.copy(pointer, value, type.class.valueTypeSize), pointer;
      case Il2Cpp3.Type.Enum.OBJECT:
      case Il2Cpp3.Type.Enum.CLASS:
      case Il2Cpp3.Type.Enum.GENERIC_INSTANCE:
        return value instanceof Il2Cpp3.ValueType ? (Memory.copy(pointer, value, type.class.valueTypeSize), pointer) : pointer.writePointer(value);
    }
    raise(`couldn't write value ${value} to ${pointer} using an unhandled or unknown type ${type.name} (${type.enumValue}), please file an issue`);
  }
  Il2Cpp3.write = write;
  function fromFridaValue(value, type) {
    if (globalThis.Array.isArray(value)) {
      const handle = Memory.alloc(type.class.valueTypeSize);
      const fields = type.class.fields.filter((_) => !_.isStatic);
      for (let i = 0; i < fields.length; i++) {
        const convertedValue = fromFridaValue(value[i], fields[i].type);
        write(handle.add(fields[i].offset).sub(Il2Cpp3.Object.headerSize), convertedValue, fields[i].type);
      }
      return new Il2Cpp3.ValueType(handle, type);
    } else if (value instanceof NativePointer) {
      if (type.isByReference) {
        return new Il2Cpp3.Reference(value, type);
      }
      switch (type.enumValue) {
        case Il2Cpp3.Type.Enum.POINTER:
          return new Il2Cpp3.Pointer(value, type.class.baseType);
        case Il2Cpp3.Type.Enum.STRING:
          return new Il2Cpp3.String(value);
        case Il2Cpp3.Type.Enum.CLASS:
        case Il2Cpp3.Type.Enum.GENERIC_INSTANCE:
        case Il2Cpp3.Type.Enum.OBJECT:
          return new Il2Cpp3.Object(value);
        case Il2Cpp3.Type.Enum.ARRAY:
        case Il2Cpp3.Type.Enum.NARRAY:
          return new Il2Cpp3.Array(value);
        default:
          return value;
      }
    } else if (type.enumValue == Il2Cpp3.Type.Enum.BOOLEAN) {
      return !!value;
    } else if (type.enumValue == Il2Cpp3.Type.Enum.VALUE_TYPE && type.class.isEnum) {
      return fromFridaValue([value], type);
    } else {
      return value;
    }
  }
  Il2Cpp3.fromFridaValue = fromFridaValue;
  function toFridaValue(value) {
    if (typeof value == "boolean") {
      return +value;
    } else if (value instanceof Il2Cpp3.ValueType) {
      if (value.type.class.isEnum) {
        return value.field("value__").value;
      } else {
        const _ = value.type.class.fields.filter((_2) => !_2.isStatic).map((_2) => toFridaValue(_2.bind(value).value));
        return _.length == 0 ? [0] : _;
      }
    } else {
      return value;
    }
  }
  Il2Cpp3.toFridaValue = toFridaValue;
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  getter(Il2Cpp3, "module", () => {
    return tryModule() ?? raise("Could not find IL2CPP module");
  });
  async function initialize(blocking = false) {
    const module = tryModule() ?? await new Promise((resolve) => {
      const [moduleName, fallbackModuleName] = getExpectedModuleNames();
      const timeout = setTimeout(() => {
        warn(`after 10 seconds, IL2CPP module '${moduleName}' has not been loaded yet, is the app running?`);
      }, 1e4);
      const moduleObserver = Process.attachModuleObserver({
        onAdded(module2) {
          if (module2.name == moduleName || fallbackModuleName && module2.name == fallbackModuleName) {
            clearTimeout(timeout);
            setImmediate(() => {
              resolve(module2);
              moduleObserver.detach();
            });
          }
        }
      });
    });
    Reflect.defineProperty(Il2Cpp3, "module", { value: module });
    if (Il2Cpp3.exports.getCorlib().isNull()) {
      return await new Promise((resolve) => {
        const timeout = setTimeout(() => {
          if (!Il2Cpp3.exports.getCorlib().isNull()) {
            warn(`resuming execution despite IL2CPP initialization not being captured in time, please open an issue as this is suboptimal`);
            interceptor.detach();
            resolve(false);
          }
        }, 1e3);
        const interceptor = Interceptor.attach(Il2Cpp3.exports.initialize, {
          onEnter() {
            clearTimeout(timeout);
          },
          onLeave() {
            interceptor.detach();
            blocking ? resolve(true) : setImmediate(() => resolve(false));
          }
        });
      });
    }
    return false;
  }
  Il2Cpp3.initialize = initialize;
  function tryModule() {
    const [moduleName, fallback] = getExpectedModuleNames();
    return Process.findModuleByName(moduleName) ?? Process.findModuleByName(fallback ?? moduleName) ?? (Process.platform == "darwin" ? Process.findModuleByAddress(DebugSymbol.fromName("il2cpp_init").address) : void 0) ?? void 0;
  }
  function getExpectedModuleNames() {
    if (Il2Cpp3.$config.moduleName) {
      return [Il2Cpp3.$config.moduleName];
    }
    switch (Process.platform) {
      case "linux":
        return [Android.apiLevel ? "libil2cpp.so" : "GameAssembly.so"];
      case "windows":
        return ["GameAssembly.dll"];
      case "darwin":
        return ["UnityFramework", "GameAssembly.dylib"];
    }
    raise(`${Process.platform} is not supported yet`);
  }
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  function nullable(valueOrNull, klass) {
    const actualClass = typeof valueOrNull == "boolean" ? Il2Cpp3.corlib.class("System.Boolean") : typeof valueOrNull == "number" ? klass ?? Il2Cpp3.corlib.class("System.Int32") : valueOrNull instanceof Int64 ? Il2Cpp3.corlib.class("System.Int64") : valueOrNull instanceof UInt64 ? Il2Cpp3.corlib.class("System.UInt64") : valueOrNull instanceof NativePointer ? klass ?? Il2Cpp3.corlib.class("System.IntPtr") : valueOrNull instanceof Il2Cpp3.ValueType ? valueOrNull.type.class : klass ?? raise(`A class must be specified when constructing a nullable for value '${valueOrNull}'`);
    if (actualClass.isValueType == false) {
      raise(`Cannot create nullable value type out of a reference type '${actualClass.type.name}'`);
    }
    const inflatedClass = Il2Cpp3.corlib.class("System.Nullable`1").inflate(actualClass);
    const struct = new Il2Cpp3.ValueType(Memory.alloc(inflatedClass.valueTypeSize), inflatedClass.type);
    (struct.tryField("hasValue") ?? struct.field("has_value")).value = valueOrNull != null;
    if (valueOrNull != null) {
      struct.field("value").value = valueOrNull;
    }
    return struct;
  }
  Il2Cpp3.nullable = nullable;
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  async function perform2(block, flag = "bind") {
    let attachedThread = null;
    try {
      const isInMainThread = await Il2Cpp3.initialize(flag == "main");
      if (flag == "main" && !isInMainThread) {
        return perform2(() => Il2Cpp3.mainThread.schedule(block), "free");
      }
      if (Il2Cpp3.currentThread == null) {
        attachedThread = Il2Cpp3.domain.attach();
      }
      if (flag == "bind" && attachedThread != null) {
        Script.bindWeak(globalThis, () => attachedThread?.detach());
      }
      const result = block();
      return result instanceof Promise ? await result : result;
    } catch (error) {
      Script.nextTick((_) => {
        throw _;
      }, error);
      return Promise.reject(error);
    } finally {
      if (flag == "free" && attachedThread != null) {
        attachedThread.detach();
      }
    }
  }
  Il2Cpp3.perform = perform2;
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  class Tracer {
    /** @internal */
    #state = {
      depth: 0,
      buffer: [],
      history: /* @__PURE__ */ new Set(),
      flush: () => {
        if (this.#state.depth == 0) {
          const message = `
${this.#state.buffer.join("\n")}
`;
          if (this.#verbose) {
            inform(message);
          } else {
            const hash = Checksum.compute("md5", message);
            if (!this.#state.history.has(hash)) {
              this.#state.history.add(hash);
              inform(message);
            }
          }
          this.#state.buffer.length = 0;
        }
      }
    };
    /** @internal */
    #threadId = Il2Cpp3.mainThread.id;
    /** @internal */
    #verbose = false;
    /** @internal */
    #applier;
    /** @internal */
    #targets = [];
    /** @internal */
    #domain;
    /** @internal */
    #assemblies;
    /** @internal */
    #classes;
    /** @internal */
    #methods;
    /** @internal */
    #assemblyFilter;
    /** @internal */
    #classFilter;
    /** @internal */
    #methodFilter;
    /** @internal */
    #parameterFilter;
    constructor(applier) {
      this.#applier = applier;
    }
    /** */
    thread(thread) {
      this.#threadId = thread.id;
      return this;
    }
    /** Determines whether print duplicate logs. */
    verbose(value) {
      this.#verbose = value;
      return this;
    }
    /** Sets the application domain as the place where to find the target methods. */
    domain() {
      this.#domain = Il2Cpp3.domain;
      return this;
    }
    /** Sets the passed `assemblies` as the place where to find the target methods. */
    assemblies(...assemblies) {
      this.#assemblies = assemblies;
      return this;
    }
    /** Sets the passed `classes` as the place where to find the target methods. */
    classes(...classes) {
      this.#classes = classes;
      return this;
    }
    /** Sets the passed `methods` as the target methods. */
    methods(...methods) {
      this.#methods = methods;
      return this;
    }
    /** Filters the assemblies where to find the target methods. */
    filterAssemblies(filter) {
      this.#assemblyFilter = filter;
      return this;
    }
    /** Filters the classes where to find the target methods. */
    filterClasses(filter) {
      this.#classFilter = filter;
      return this;
    }
    /** Filters the target methods. */
    filterMethods(filter) {
      this.#methodFilter = filter;
      return this;
    }
    /** Filters the target methods. */
    filterParameters(filter) {
      this.#parameterFilter = filter;
      return this;
    }
    /** Commits the current changes by finding the target methods. */
    and() {
      const filterMethod = (method) => {
        if (this.#parameterFilter == void 0) {
          this.#targets.push(method);
          return;
        }
        for (const parameter of method.parameters) {
          if (this.#parameterFilter(parameter)) {
            this.#targets.push(method);
            break;
          }
        }
      };
      const filterMethods = (values) => {
        for (const method of values) {
          filterMethod(method);
        }
      };
      const filterClass = (klass) => {
        if (this.#methodFilter == void 0) {
          filterMethods(klass.methods);
          return;
        }
        for (const method of klass.methods) {
          if (this.#methodFilter(method)) {
            filterMethod(method);
          }
        }
      };
      const filterClasses = (values) => {
        for (const klass of values) {
          filterClass(klass);
        }
      };
      const filterAssembly = (assembly) => {
        if (this.#classFilter == void 0) {
          filterClasses(assembly.image.classes);
          return;
        }
        for (const klass of assembly.image.classes) {
          if (this.#classFilter(klass)) {
            filterClass(klass);
          }
        }
      };
      const filterAssemblies = (assemblies) => {
        for (const assembly of assemblies) {
          filterAssembly(assembly);
        }
      };
      const filterDomain = (domain) => {
        if (this.#assemblyFilter == void 0) {
          filterAssemblies(domain.assemblies);
          return;
        }
        for (const assembly of domain.assemblies) {
          if (this.#assemblyFilter(assembly)) {
            filterAssembly(assembly);
          }
        }
      };
      this.#methods ? filterMethods(this.#methods) : this.#classes ? filterClasses(this.#classes) : this.#assemblies ? filterAssemblies(this.#assemblies) : this.#domain ? filterDomain(this.#domain) : void 0;
      this.#assemblies = void 0;
      this.#classes = void 0;
      this.#methods = void 0;
      this.#assemblyFilter = void 0;
      this.#classFilter = void 0;
      this.#methodFilter = void 0;
      this.#parameterFilter = void 0;
      return this;
    }
    /** Starts tracing. */
    attach() {
      for (const target of this.#targets) {
        if (!target.virtualAddress.isNull()) {
          try {
            this.#applier(target, this.#state, this.#threadId);
          } catch (e) {
            switch (e.message) {
              case /unable to intercept function at \w+; please file a bug/.exec(e.message)?.input:
              case "already replaced this function":
                break;
              default:
                throw e;
            }
          }
        }
      }
    }
  }
  Il2Cpp3.Tracer = Tracer;
  function trace(parameters = false) {
    const applier = () => (method, state, threadId) => {
      const paddedVirtualAddress = method.relativeVirtualAddress.toString(16).padStart(8, "0");
      Interceptor.attach(method.virtualAddress, {
        onEnter() {
          if (this.threadId == threadId) {
            state.buffer.push(`\x1B[2m0x${paddedVirtualAddress}\x1B[0m ${`\u2502 `.repeat(state.depth++)}\u250C\u2500\x1B[35m${method.class.type.name}::\x1B[1m${method.name}\x1B[0m\x1B[0m`);
          }
        },
        onLeave() {
          if (this.threadId == threadId) {
            state.buffer.push(`\x1B[2m0x${paddedVirtualAddress}\x1B[0m ${`\u2502 `.repeat(--state.depth)}\u2514\u2500\x1B[33m${method.class.type.name}::\x1B[1m${method.name}\x1B[0m\x1B[0m`);
            state.flush();
          }
        }
      });
    };
    const applierWithParameters = () => (method, state, threadId) => {
      const paddedVirtualAddress = method.relativeVirtualAddress.toString(16).padStart(8, "0");
      const parameterStartIndex = +method.nativeSignatureHasInstanceSlot;
      const callback = function(...args) {
        if (this.threadId == threadId) {
          const thisParameter = method.isStatic ? void 0 : new Il2Cpp3.Parameter("this", -1, method.class.type);
          const parameters2 = thisParameter ? [thisParameter].concat(method.parameters) : method.parameters;
          state.buffer.push(`\x1B[2m0x${paddedVirtualAddress}\x1B[0m ${`\u2502 `.repeat(state.depth++)}\u250C\u2500\x1B[35m${method.class.type.name}::\x1B[1m${method.name}\x1B[0m\x1B[0m(${parameters2.map((e) => `\x1B[32m${e.name}\x1B[0m = \x1B[31m${Il2Cpp3.fromFridaValue(args[e.position + parameterStartIndex], e.type)}\x1B[0m`).join(", ")})`);
        }
        const returnValue = method.nativeFunction(...args);
        if (this.threadId == threadId) {
          state.buffer.push(`\x1B[2m0x${paddedVirtualAddress}\x1B[0m ${`\u2502 `.repeat(--state.depth)}\u2514\u2500\x1B[33m${method.class.type.name}::\x1B[1m${method.name}\x1B[0m\x1B[0m${returnValue == void 0 ? "" : ` = \x1B[36m${Il2Cpp3.fromFridaValue(returnValue, method.returnType)}`}\x1B[0m`);
          state.flush();
        }
        return returnValue;
      };
      method.revert();
      const nativeCallback = new NativeCallback(callback, method.returnType.fridaAlias, method.fridaSignature);
      Interceptor.replace(method.virtualAddress, nativeCallback);
    };
    return new Il2Cpp3.Tracer(parameters ? applierWithParameters() : applier());
  }
  Il2Cpp3.trace = trace;
  function backtrace(mode) {
    const methods = Il2Cpp3.domain.assemblies.flatMap((_) => _.image.classes.flatMap((_2) => _2.methods.filter((_3) => !_3.virtualAddress.isNull()))).sort((_, __) => _.virtualAddress.compare(__.virtualAddress));
    const searchInsert = (target) => {
      let left = 0;
      let right = methods.length - 1;
      while (left <= right) {
        const pivot = Math.floor((left + right) / 2);
        const comparison = methods[pivot].virtualAddress.compare(target);
        if (comparison == 0) {
          return methods[pivot];
        } else if (comparison > 0) {
          right = pivot - 1;
        } else {
          left = pivot + 1;
        }
      }
      return methods[right];
    };
    const applier = () => (method, state, threadId) => {
      Interceptor.attach(method.virtualAddress, function() {
        if (this.threadId == threadId) {
          const handles = globalThis.Thread.backtrace(this.context, mode);
          handles.unshift(method.virtualAddress);
          for (const handle of handles) {
            if (handle.compare(Il2Cpp3.module.base) > 0 && handle.compare(Il2Cpp3.module.base.add(Il2Cpp3.module.size)) < 0) {
              const method2 = searchInsert(handle);
              if (method2) {
                const offset = handle.sub(method2.virtualAddress);
                if (offset.compare(4095) < 0) {
                  state.buffer.push(`\x1B[2m0x${method2.relativeVirtualAddress.toString(16).padStart(8, "0")}\x1B[0m\x1B[2m+0x${offset.toString(16).padStart(3, `0`)}\x1B[0m ${method2.class.type.name}::\x1B[1m${method2.name}\x1B[0m`);
                }
              }
            }
          }
          state.flush();
        }
      });
    };
    return new Il2Cpp3.Tracer(applier());
  }
  Il2Cpp3.backtrace = backtrace;
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  class Array2 extends NativeStruct {
    /** Gets the Il2CppArray struct size, possibly equal to `Process.pointerSize * 4`. */
    static get headerSize() {
      return Il2Cpp3.corlib.class("System.Array").instanceSize;
    }
    /** @internal Gets a pointer to the first element of the current array. */
    get elements() {
      const array2 = Il2Cpp3.string("vfsfitvnm").object.method("ToCharArray", 0).invoke();
      const offset = Memory.scanSync(array2.handle, 255, "76 00 66 00 73 00 66 00 69 00 74 00 76 00 6e 00 6d 00")[0]?.address?.sub(array2.handle) ?? raise("couldn't find the elements offset in the native array struct");
      getter(Il2Cpp3.Array.prototype, "elements", function() {
        return new Il2Cpp3.Pointer(this.handle.add(offset), this.elementType);
      }, lazy);
      return this.elements;
    }
    /** Gets the size of the object encompassed by the current array. */
    get elementSize() {
      return this.elementType.class.arrayElementSize;
    }
    /** Gets the type of the object encompassed by the current array. */
    get elementType() {
      return this.object.class.type.class.baseType;
    }
    /** Gets the total number of elements in all the dimensions of the current array. */
    get length() {
      return Il2Cpp3.exports.arrayGetLength(this);
    }
    /** Gets the encompassing object of the current array. */
    get object() {
      return new Il2Cpp3.Object(this);
    }
    /** Gets the element at the specified index of the current array. */
    get(index) {
      if (index < 0 || index >= this.length) {
        raise(`cannot get element at index ${index} as the array length is ${this.length}`);
      }
      return this.elements.get(index);
    }
    /** Sets the element at the specified index of the current array. */
    set(index, value) {
      if (index < 0 || index >= this.length) {
        raise(`cannot set element at index ${index} as the array length is ${this.length}`);
      }
      this.elements.set(index, value);
    }
    /** */
    toString() {
      return this.isNull() ? "null" : `[${this.elements.read(this.length, 0)}]`;
    }
    /** Iterable. */
    *[Symbol.iterator]() {
      for (let i = 0; i < this.length; i++) {
        yield this.elements.get(i);
      }
    }
  }
  __decorate([
    lazy
  ], Array2.prototype, "elementSize", null);
  __decorate([
    lazy
  ], Array2.prototype, "elementType", null);
  __decorate([
    lazy
  ], Array2.prototype, "length", null);
  __decorate([
    lazy
  ], Array2.prototype, "object", null);
  __decorate([
    lazy
  ], Array2, "headerSize", null);
  Il2Cpp3.Array = Array2;
  function array(klass, lengthOrElements) {
    const length = typeof lengthOrElements == "number" ? lengthOrElements : lengthOrElements.length;
    const array2 = new Il2Cpp3.Array(Il2Cpp3.exports.arrayNew(klass, length));
    if (globalThis.Array.isArray(lengthOrElements)) {
      array2.elements.write(lengthOrElements);
    }
    return array2;
  }
  Il2Cpp3.array = array;
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  let Assembly = class Assembly extends NativeStruct {
    /** Gets the image of this assembly. */
    get image() {
      if (Il2Cpp3.exports.assemblyGetImage.isNull()) {
        const runtimeModule = this.object.tryMethod("GetType", 1)?.invoke(Il2Cpp3.string("<Module>"))?.asNullable()?.tryMethod("get_Module")?.invoke() ?? this.object.tryMethod("GetModules", 1)?.invoke(false)?.get(0) ?? raise(`couldn't find the runtime module object of assembly ${this.name}`);
        return new Il2Cpp3.Image(runtimeModule.field("_impl").value);
      }
      return new Il2Cpp3.Image(Il2Cpp3.exports.assemblyGetImage(this));
    }
    /** Gets the name of this assembly. */
    get name() {
      return this.image.name.replace(".dll", "");
    }
    /** Gets the encompassing object of the current assembly. */
    get object() {
      for (const _ of Il2Cpp3.domain.object.method("GetAssemblies", 1).invoke(false)) {
        if (_.field("_mono_assembly").value.equals(this)) {
          return _;
        }
      }
      raise("couldn't find the object of the native assembly struct");
    }
  };
  __decorate([
    lazy
  ], Assembly.prototype, "name", null);
  __decorate([
    lazy
  ], Assembly.prototype, "object", null);
  Assembly = __decorate([
    recycle
  ], Assembly);
  Il2Cpp3.Assembly = Assembly;
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  let Class = class Class extends NativeStruct {
    /** Gets the actual size of the instance of the current class. */
    get actualInstanceSize() {
      const SystemString = Il2Cpp3.corlib.class("System.String");
      const offset = SystemString.handle.offsetOf((_) => _.readInt() == SystemString.instanceSize - 2) ?? raise("couldn't find the actual instance size offset in the native class struct");
      getter(Il2Cpp3.Class.prototype, "actualInstanceSize", function() {
        return this.handle.add(offset).readS32();
      }, lazy);
      return this.actualInstanceSize;
    }
    /** Gets the array class which encompass the current class. */
    get arrayClass() {
      return new Il2Cpp3.Class(Il2Cpp3.exports.classGetArrayClass(this, 1));
    }
    /** Gets the size of the object encompassed by the current array class. */
    get arrayElementSize() {
      return Il2Cpp3.exports.classGetArrayElementSize(this);
    }
    /** Gets the name of the assembly in which the current class is defined. */
    get assemblyName() {
      return Il2Cpp3.exports.classGetAssemblyName(this).readUtf8String().replace(".dll", "");
    }
    /** Gets the class that declares the current nested class. */
    get declaringClass() {
      return new Il2Cpp3.Class(Il2Cpp3.exports.classGetDeclaringType(this)).asNullable();
    }
    /** Gets the encompassed type of this array, reference, pointer or enum type. */
    get baseType() {
      return new Il2Cpp3.Type(Il2Cpp3.exports.classGetBaseType(this)).asNullable();
    }
    /** Gets the class of the object encompassed or referred to by the current array, pointer or reference class. */
    get elementClass() {
      return new Il2Cpp3.Class(Il2Cpp3.exports.classGetElementClass(this)).asNullable();
    }
    /** Gets the fields of the current class. */
    get fields() {
      return readNativeIterator((_) => Il2Cpp3.exports.classGetFields(this, _)).map((_) => new Il2Cpp3.Field(_));
    }
    /** Gets the flags of the current class. */
    get flags() {
      return Il2Cpp3.exports.classGetFlags(this);
    }
    /** Gets the full name (namespace + name) of the current class. */
    get fullName() {
      return this.namespace ? `${this.namespace}.${this.name}` : this.name;
    }
    /** Gets the generic class of the current class if the current class is inflated. */
    get genericClass() {
      const klass = this.image.tryClass(this.fullName)?.asNullable();
      return klass?.equals(this) ? null : klass ?? null;
    }
    /** Gets the generics parameters of this generic class. */
    get generics() {
      if (!this.isGeneric && !this.isInflated) {
        return [];
      }
      const types = this.type.object.method("GetGenericArguments").invoke();
      return globalThis.Array.from(types).map((_) => new Il2Cpp3.Class(Il2Cpp3.exports.classFromObject(_)));
    }
    /** Determines whether the GC has tracking references to the current class instances. */
    get hasReferences() {
      return !!Il2Cpp3.exports.classHasReferences(this);
    }
    /** Determines whether ther current class has a valid static constructor. */
    get hasStaticConstructor() {
      const staticConstructor = this.tryMethod(".cctor");
      return staticConstructor != null && !staticConstructor.virtualAddress.isNull();
    }
    /** Gets the image in which the current class is defined. */
    get image() {
      return new Il2Cpp3.Image(Il2Cpp3.exports.classGetImage(this));
    }
    /** Gets the size of the instance of the current class. */
    get instanceSize() {
      return Il2Cpp3.exports.classGetInstanceSize(this);
    }
    /** Determines whether the current class is abstract. */
    get isAbstract() {
      return !!Il2Cpp3.exports.classIsAbstract(this);
    }
    /** Determines whether the current class is blittable. */
    get isBlittable() {
      return !!Il2Cpp3.exports.classIsBlittable(this);
    }
    /** Determines whether the current class is an enumeration. */
    get isEnum() {
      return !!Il2Cpp3.exports.classIsEnum(this);
    }
    /** Determines whether the current class is a generic one. */
    get isGeneric() {
      return !!Il2Cpp3.exports.classIsGeneric(this);
    }
    /** Determines whether the current class is inflated. */
    get isInflated() {
      return !!Il2Cpp3.exports.classIsInflated(this);
    }
    /** Determines whether the current class is an interface. */
    get isInterface() {
      return !!Il2Cpp3.exports.classIsInterface(this);
    }
    /** Determines whether the current class is a struct. */
    get isStruct() {
      return this.isValueType && !this.isEnum;
    }
    /** Determines whether the current class is a value type. */
    get isValueType() {
      return !!Il2Cpp3.exports.classIsValueType(this);
    }
    /** Gets the interfaces implemented or inherited by the current class. */
    get interfaces() {
      return readNativeIterator((_) => Il2Cpp3.exports.classGetInterfaces(this, _)).map((_) => new Il2Cpp3.Class(_));
    }
    /** Gets the methods implemented by the current class. */
    get methods() {
      return readNativeIterator((_) => Il2Cpp3.exports.classGetMethods(this, _)).map((_) => new Il2Cpp3.Method(_));
    }
    /** Gets the name of the current class. */
    get name() {
      return Il2Cpp3.exports.classGetName(this).readUtf8String();
    }
    /** Gets the namespace of the current class. */
    get namespace() {
      return Il2Cpp3.exports.classGetNamespace(this).readUtf8String() || void 0;
    }
    /** Gets the classes nested inside the current class. */
    get nestedClasses() {
      return readNativeIterator((_) => Il2Cpp3.exports.classGetNestedClasses(this, _)).map((_) => new Il2Cpp3.Class(_));
    }
    /** Gets the class from which the current class directly inherits. */
    get parent() {
      return new Il2Cpp3.Class(Il2Cpp3.exports.classGetParent(this)).asNullable();
    }
    /** Gets the pointer class of the current class. */
    get pointerClass() {
      return new Il2Cpp3.Class(Il2Cpp3.exports.classFromObject(this.type.object.method("MakePointerType").invoke()));
    }
    /** Gets the rank (number of dimensions) of the current array class. */
    get rank() {
      let rank = 0;
      const name = this.name;
      for (let i = this.name.length - 1; i > 0; i--) {
        const c = name[i];
        if (c == "]")
          rank++;
        else if (c == "[" || rank == 0)
          break;
        else if (c == ",")
          rank++;
        else
          break;
      }
      return rank;
    }
    /** Gets a pointer to the static fields of the current class. */
    get staticFieldsData() {
      return Il2Cpp3.exports.classGetStaticFieldData(this);
    }
    /** Gets the size of the instance - as a value type - of the current class. */
    get valueTypeSize() {
      return Il2Cpp3.exports.classGetValueTypeSize(this, NULL);
    }
    /** Gets the type of the current class. */
    get type() {
      return new Il2Cpp3.Type(Il2Cpp3.exports.classGetType(this));
    }
    /** Allocates a new object of the current class. */
    alloc() {
      return new Il2Cpp3.Object(Il2Cpp3.exports.objectNew(this));
    }
    /** Gets the field identified by the given name. */
    field(name) {
      return this.tryField(name) ?? raise(`couldn't find field ${name} in class ${this.type.name}`);
    }
    /** Gets the hierarchy of the current class. */
    *hierarchy(options) {
      let klass = options?.includeCurrent ?? true ? this : this.parent;
      while (klass) {
        yield klass;
        klass = klass.parent;
      }
    }
    /** Builds a generic instance of the current generic class. */
    inflate(...classes) {
      if (!this.isGeneric) {
        raise(`cannot inflate class ${this.type.name} as it has no generic parameters`);
      }
      if (this.generics.length != classes.length) {
        raise(`cannot inflate class ${this.type.name} as it needs ${this.generics.length} generic parameter(s), not ${classes.length}`);
      }
      const types = classes.map((_) => _.type.object);
      const typeArray = Il2Cpp3.array(Il2Cpp3.corlib.class("System.Type"), types);
      const inflatedType = this.type.object.method("MakeGenericType", 1).invoke(typeArray);
      return new Il2Cpp3.Class(Il2Cpp3.exports.classFromObject(inflatedType));
    }
    /** Calls the static constructor of the current class. */
    initialize() {
      Il2Cpp3.exports.classInitialize(this);
      return this;
    }
    /** Determines whether an instance of `other` class can be assigned to a variable of the current type. */
    isAssignableFrom(other) {
      return !!Il2Cpp3.exports.classIsAssignableFrom(this, other);
    }
    /** Determines whether the current class derives from `other` class. */
    isSubclassOf(other, checkInterfaces) {
      return !!Il2Cpp3.exports.classIsSubclassOf(this, other, +checkInterfaces);
    }
    /** Gets the method identified by the given name and parameter count. */
    method(name, parameterCount = -1) {
      return this.tryMethod(name, parameterCount) ?? raise(`couldn't find method ${name} in class ${this.type.name}`);
    }
    /** Gets the nested class with the given name. */
    nested(name) {
      return this.tryNested(name) ?? raise(`couldn't find nested class ${name} in class ${this.type.name}`);
    }
    /** Allocates a new object of the current class and calls its default constructor. */
    new() {
      const object = this.alloc();
      const exceptionArray = Memory.alloc(Process.pointerSize);
      Il2Cpp3.exports.objectInitialize(object, exceptionArray);
      const exception = exceptionArray.readPointer();
      if (!exception.isNull()) {
        raise(new Il2Cpp3.Object(exception).toString());
      }
      return object;
    }
    /** Gets the field with the given name. */
    tryField(name) {
      return new Il2Cpp3.Field(Il2Cpp3.exports.classGetFieldFromName(this, Memory.allocUtf8String(name))).asNullable();
    }
    /** Gets the method with the given name and parameter count. */
    tryMethod(name, parameterCount = -1) {
      return new Il2Cpp3.Method(Il2Cpp3.exports.classGetMethodFromName(this, Memory.allocUtf8String(name), parameterCount)).asNullable();
    }
    /** Gets the nested class with the given name. */
    tryNested(name) {
      return this.nestedClasses.find((_) => _.name == name);
    }
    /** */
    toString() {
      const inherited = [this.parent].concat(this.interfaces);
      return `// ${this.assemblyName}
${this.isEnum ? `enum` : this.isStruct ? `struct` : this.isInterface ? `interface` : `class`} ${this.type.name}${inherited ? ` : ${inherited.map((_) => _?.type.name).join(`, `)}` : ``}
{
    ${this.fields.join(`
    `)}
    ${this.methods.join(`
    `)}
}`;
    }
    /** Executes a callback for every defined class. */
    static enumerate(block) {
      const callback = new NativeCallback((_) => block(new Il2Cpp3.Class(_)), "void", ["pointer", "pointer"]);
      return Il2Cpp3.exports.classForEach(callback, NULL);
    }
  };
  __decorate([
    lazy
  ], Class.prototype, "arrayClass", null);
  __decorate([
    lazy
  ], Class.prototype, "arrayElementSize", null);
  __decorate([
    lazy
  ], Class.prototype, "assemblyName", null);
  __decorate([
    lazy
  ], Class.prototype, "declaringClass", null);
  __decorate([
    lazy
  ], Class.prototype, "baseType", null);
  __decorate([
    lazy
  ], Class.prototype, "elementClass", null);
  __decorate([
    lazy
  ], Class.prototype, "fields", null);
  __decorate([
    lazy
  ], Class.prototype, "flags", null);
  __decorate([
    lazy
  ], Class.prototype, "fullName", null);
  __decorate([
    lazy
  ], Class.prototype, "generics", null);
  __decorate([
    lazy
  ], Class.prototype, "hasReferences", null);
  __decorate([
    lazy
  ], Class.prototype, "hasStaticConstructor", null);
  __decorate([
    lazy
  ], Class.prototype, "image", null);
  __decorate([
    lazy
  ], Class.prototype, "instanceSize", null);
  __decorate([
    lazy
  ], Class.prototype, "isAbstract", null);
  __decorate([
    lazy
  ], Class.prototype, "isBlittable", null);
  __decorate([
    lazy
  ], Class.prototype, "isEnum", null);
  __decorate([
    lazy
  ], Class.prototype, "isGeneric", null);
  __decorate([
    lazy
  ], Class.prototype, "isInflated", null);
  __decorate([
    lazy
  ], Class.prototype, "isInterface", null);
  __decorate([
    lazy
  ], Class.prototype, "isValueType", null);
  __decorate([
    lazy
  ], Class.prototype, "interfaces", null);
  __decorate([
    lazy
  ], Class.prototype, "methods", null);
  __decorate([
    lazy
  ], Class.prototype, "name", null);
  __decorate([
    lazy
  ], Class.prototype, "namespace", null);
  __decorate([
    lazy
  ], Class.prototype, "nestedClasses", null);
  __decorate([
    lazy
  ], Class.prototype, "parent", null);
  __decorate([
    lazy
  ], Class.prototype, "pointerClass", null);
  __decorate([
    lazy
  ], Class.prototype, "rank", null);
  __decorate([
    lazy
  ], Class.prototype, "staticFieldsData", null);
  __decorate([
    lazy
  ], Class.prototype, "valueTypeSize", null);
  __decorate([
    lazy
  ], Class.prototype, "type", null);
  Class = __decorate([
    recycle
  ], Class);
  Il2Cpp3.Class = Class;
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  function delegate(klass, block) {
    const SystemDelegate = Il2Cpp3.corlib.class("System.Delegate");
    const SystemMulticastDelegate = Il2Cpp3.corlib.class("System.MulticastDelegate");
    if (!SystemDelegate.isAssignableFrom(klass)) {
      raise(`cannot create a delegate for ${klass.type.name} as it's a non-delegate class`);
    }
    if (klass.equals(SystemDelegate) || klass.equals(SystemMulticastDelegate)) {
      raise(`cannot create a delegate for neither ${SystemDelegate.type.name} nor ${SystemMulticastDelegate.type.name}, use a subclass instead`);
    }
    const delegate2 = klass.alloc();
    const key = delegate2.handle.toString();
    const Invoke = delegate2.tryMethod("Invoke") ?? raise(`cannot create a delegate for ${klass.type.name}, there is no Invoke method`);
    delegate2.method(".ctor").invoke(delegate2, Invoke.handle);
    const callback = Invoke.wrap(block);
    delegate2.field("method_ptr").value = callback;
    delegate2.field("invoke_impl").value = callback;
    Il2Cpp3._callbacksToKeepAlive[key] = callback;
    return delegate2;
  }
  Il2Cpp3.delegate = delegate;
  Il2Cpp3._callbacksToKeepAlive = {};
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  let Domain = class Domain extends NativeStruct {
    /** Gets the assemblies that have been loaded into the execution context of the application domain. */
    get assemblies() {
      let handles = readNativeList((_) => Il2Cpp3.exports.domainGetAssemblies(this, _));
      if (handles.length == 0) {
        const assemblyObjects = this.object.method("GetAssemblies").overload().invoke();
        handles = globalThis.Array.from(assemblyObjects).map((_) => _.field("_mono_assembly").value);
      }
      return handles.map((_) => new Il2Cpp3.Assembly(_));
    }
    /** Gets the encompassing object of the application domain. */
    get object() {
      return Il2Cpp3.corlib.class("System.AppDomain").method("get_CurrentDomain").invoke();
    }
    /** Opens and loads the assembly with the given name. */
    assembly(name) {
      return this.tryAssembly(name) ?? raise(`couldn't find assembly ${name}`);
    }
    /** Attached a new thread to the application domain. */
    attach() {
      return new Il2Cpp3.Thread(Il2Cpp3.exports.threadAttach(this));
    }
    /** Opens and loads the assembly with the given name. */
    tryAssembly(name) {
      return new Il2Cpp3.Assembly(Il2Cpp3.exports.domainGetAssemblyFromName(this, Memory.allocUtf8String(name))).asNullable();
    }
  };
  __decorate([
    lazy
  ], Domain.prototype, "assemblies", null);
  __decorate([
    lazy
  ], Domain.prototype, "object", null);
  Domain = __decorate([
    recycle
  ], Domain);
  Il2Cpp3.Domain = Domain;
  getter(Il2Cpp3, "domain", () => {
    return new Il2Cpp3.Domain(Il2Cpp3.exports.domainGet());
  }, lazy);
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  class Field extends NativeStruct {
    /** Gets the class in which this field is defined. */
    get class() {
      return new Il2Cpp3.Class(Il2Cpp3.exports.fieldGetClass(this));
    }
    /** Gets the flags of the current field. */
    get flags() {
      return Il2Cpp3.exports.fieldGetFlags(this);
    }
    /** Determines whether this field value is known at compile time. */
    get isLiteral() {
      return (this.flags & 64) != 0;
    }
    /** Determines whether this field is static. */
    get isStatic() {
      return (this.flags & 16) != 0;
    }
    /** Determines whether this field is thread static. */
    get isThreadStatic() {
      const offset = Il2Cpp3.corlib.class("System.AppDomain").field("type_resolve_in_progress").offset;
      getter(Il2Cpp3.Field.prototype, "isThreadStatic", function() {
        return this.offset == offset;
      }, lazy);
      return this.isThreadStatic;
    }
    /** Gets the access modifier of this field. */
    get modifier() {
      switch (this.flags & 7) {
        case 1:
          return "private";
        case 2:
          return "private protected";
        case 3:
          return "internal";
        case 4:
          return "protected";
        case 5:
          return "protected internal";
        case 6:
          return "public";
      }
    }
    /** Gets the name of this field. */
    get name() {
      return Il2Cpp3.exports.fieldGetName(this).readUtf8String();
    }
    /** Gets the offset of this field, calculated as the difference with its owner virtual address. */
    get offset() {
      return Il2Cpp3.exports.fieldGetOffset(this);
    }
    /** Gets the type of this field. */
    get type() {
      return new Il2Cpp3.Type(Il2Cpp3.exports.fieldGetType(this));
    }
    /** Gets the value of this field. */
    get value() {
      if (!this.isStatic) {
        raise(`cannot access instance field ${this.class.type.name}::${this.name} from a class, use an object instead`);
      }
      const handle = Memory.alloc(Process.pointerSize);
      Il2Cpp3.exports.fieldGetStaticValue(this.handle, handle);
      return Il2Cpp3.read(handle, this.type);
    }
    /** Sets the value of this field. Thread static or literal values cannot be altered yet. */
    set value(value) {
      if (!this.isStatic) {
        raise(`cannot access instance field ${this.class.type.name}::${this.name} from a class, use an object instead`);
      }
      if (this.isThreadStatic || this.isLiteral) {
        raise(`cannot write the value of field ${this.name} as it's thread static or literal`);
      }
      const handle = (
        // pointer-like values should be passed as-is, but boxed
        // value types (primitives included) must be unboxed first
        value instanceof Il2Cpp3.Object && this.type.class.isValueType ? value.unbox() : value instanceof NativeStruct ? value.handle : value instanceof NativePointer ? value : Il2Cpp3.write(Memory.alloc(this.type.class.valueTypeSize), value, this.type)
      );
      Il2Cpp3.exports.fieldSetStaticValue(this.handle, handle);
    }
    /** */
    toString() {
      return `${this.isThreadStatic ? `[ThreadStatic] ` : ``}${this.isStatic ? `static ` : ``}${this.type.name} ${this.name}${this.isLiteral ? ` = ${this.type.class.isEnum ? Il2Cpp3.read(this.value.handle, this.type.class.baseType) : this.value}` : ``};${this.isThreadStatic || this.isLiteral ? `` : ` // 0x${this.offset.toString(16)}`}`;
    }
    /**
     * @internal
     * Binds the current field to a {@link Il2Cpp.Object} or a
     * {@link Il2Cpp.ValueType} (also known as *instances*), so that it is
     * possible to retrieve its value - see {@link Il2Cpp.Field.value} for
     * details. \
     * Binding a static field is forbidden.
     */
    bind(instance) {
      if (this.isStatic) {
        raise(`cannot bind static field ${this.class.type.name}::${this.name} to an instance`);
      }
      const offset = this.offset - (instance instanceof Il2Cpp3.ValueType ? Il2Cpp3.Object.headerSize : 0);
      return new Proxy(this, {
        get(target, property) {
          if (property == "value") {
            return Il2Cpp3.read(instance.handle.add(offset), target.type);
          }
          return Reflect.get(target, property);
        },
        set(target, property, value) {
          if (property == "value") {
            Il2Cpp3.write(instance.handle.add(offset), value, target.type);
            return true;
          }
          return Reflect.set(target, property, value);
        }
      });
    }
  }
  __decorate([
    lazy
  ], Field.prototype, "class", null);
  __decorate([
    lazy
  ], Field.prototype, "flags", null);
  __decorate([
    lazy
  ], Field.prototype, "isLiteral", null);
  __decorate([
    lazy
  ], Field.prototype, "isStatic", null);
  __decorate([
    lazy
  ], Field.prototype, "isThreadStatic", null);
  __decorate([
    lazy
  ], Field.prototype, "modifier", null);
  __decorate([
    lazy
  ], Field.prototype, "name", null);
  __decorate([
    lazy
  ], Field.prototype, "offset", null);
  __decorate([
    lazy
  ], Field.prototype, "type", null);
  Il2Cpp3.Field = Field;
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  class GCHandle {
    handle;
    /** @internal */
    constructor(handle) {
      this.handle = handle;
    }
    /** Gets the object associated to this handle. */
    get target() {
      return new Il2Cpp3.Object(Il2Cpp3.exports.gcHandleGetTarget(this.handle)).asNullable();
    }
    /** Frees this handle. */
    free() {
      return Il2Cpp3.exports.gcHandleFree(this.handle);
    }
  }
  Il2Cpp3.GCHandle = GCHandle;
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  let Image = class Image extends NativeStruct {
    /** Gets the assembly in which the current image is defined. */
    get assembly() {
      return new Il2Cpp3.Assembly(Il2Cpp3.exports.imageGetAssembly(this));
    }
    /** Gets the amount of classes defined in this image. */
    get classCount() {
      if (Il2Cpp3.exports.imageGetClassCount.isNull()) {
        return this.classes.length;
      } else {
        return Il2Cpp3.exports.imageGetClassCount(this);
      }
    }
    /** Gets the classes defined in this image. */
    get classes() {
      if (Il2Cpp3.exports.imageGetClass.isNull()) {
        const types = this.assembly.object.method("GetTypes").invoke(false);
        const classes = globalThis.Array.from(types, (_) => new Il2Cpp3.Class(Il2Cpp3.exports.classFromObject(_)));
        const Module = this.tryClass("<Module>");
        if (Module) {
          classes.unshift(Module);
        }
        return classes;
      } else {
        return globalThis.Array.from(globalThis.Array(this.classCount), (_, i) => new Il2Cpp3.Class(Il2Cpp3.exports.imageGetClass(this, i)));
      }
    }
    /** Gets the name of this image. */
    get name() {
      return Il2Cpp3.exports.imageGetName(this).readUtf8String();
    }
    /** Gets the class with the specified name defined in this image. */
    class(name) {
      return this.tryClass(name) ?? raise(`couldn't find class ${name} in assembly ${this.name}`);
    }
    /** Gets the class with the specified name defined in this image. */
    tryClass(name) {
      const dotIndex = name.lastIndexOf(".");
      const classNamespace = Memory.allocUtf8String(dotIndex == -1 ? "" : name.slice(0, dotIndex));
      const className = Memory.allocUtf8String(name.slice(dotIndex + 1));
      return new Il2Cpp3.Class(Il2Cpp3.exports.classFromName(this, classNamespace, className)).asNullable();
    }
  };
  __decorate([
    lazy
  ], Image.prototype, "assembly", null);
  __decorate([
    lazy
  ], Image.prototype, "classCount", null);
  __decorate([
    lazy
  ], Image.prototype, "classes", null);
  __decorate([
    lazy
  ], Image.prototype, "name", null);
  Image = __decorate([
    recycle
  ], Image);
  Il2Cpp3.Image = Image;
  getter(Il2Cpp3, "corlib", () => {
    return new Il2Cpp3.Image(Il2Cpp3.exports.getCorlib());
  }, lazy);
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  class MemorySnapshot extends NativeStruct {
    /** Captures a memory snapshot. */
    static capture() {
      return new Il2Cpp3.MemorySnapshot();
    }
    /** Creates a memory snapshot with the given handle. */
    constructor(handle = Il2Cpp3.exports.memorySnapshotCapture()) {
      super(handle);
    }
    /** Gets any initialized class. */
    get classes() {
      return readNativeIterator((_) => Il2Cpp3.exports.memorySnapshotGetClasses(this, _)).map((_) => new Il2Cpp3.Class(_));
    }
    /** Gets the objects tracked by this memory snapshot. */
    get objects() {
      return readNativeList((_) => Il2Cpp3.exports.memorySnapshotGetObjects(this, _)).filter((_) => !_.isNull()).map((_) => new Il2Cpp3.Object(_));
    }
    /** Frees this memory snapshot. */
    free() {
      Il2Cpp3.exports.memorySnapshotFree(this);
    }
  }
  __decorate([
    lazy
  ], MemorySnapshot.prototype, "classes", null);
  __decorate([
    lazy
  ], MemorySnapshot.prototype, "objects", null);
  Il2Cpp3.MemorySnapshot = MemorySnapshot;
  function memorySnapshot(block) {
    const memorySnapshot2 = Il2Cpp3.MemorySnapshot.capture();
    const result = block(memorySnapshot2);
    memorySnapshot2.free();
    return result;
  }
  Il2Cpp3.memorySnapshot = memorySnapshot;
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  class Method extends NativeStruct {
    /** Gets the class in which this method is defined. */
    get class() {
      return new Il2Cpp3.Class(Il2Cpp3.exports.methodGetClass(this));
    }
    /** Gets the flags of the current method. */
    get flags() {
      return Il2Cpp3.exports.methodGetFlags(this, NULL);
    }
    /** Gets the implementation flags of the current method. */
    get implementationFlags() {
      const implementationFlagsPointer = Memory.alloc(Process.pointerSize);
      Il2Cpp3.exports.methodGetFlags(this, implementationFlagsPointer);
      return implementationFlagsPointer.readU32();
    }
    /** */
    get fridaSignature() {
      const types = [];
      for (const parameter of this.parameters) {
        types.push(parameter.type.fridaAlias);
      }
      if (this.nativeSignatureHasInstanceSlot) {
        types.unshift("pointer");
      }
      if (this.isInflated) {
        types.push("pointer");
      }
      return types;
    }
    /** Gets the generic parameters of this generic method. */
    get generics() {
      if (!this.isGeneric) {
        return [];
      }
      const types = this.object.method("GetGenericArguments").invoke();
      return globalThis.Array.from(types).map((_) => new Il2Cpp3.Class(Il2Cpp3.exports.classFromObject(_)));
    }
    /** Determines whether this method is external. */
    get isExternal() {
      return (this.implementationFlags & 4096) != 0;
    }
    /** Determines whether this method is generic. */
    get isGeneric() {
      return !!Il2Cpp3.exports.methodIsGeneric(this);
    }
    /** Determines whether this method is inflated (generic with a concrete type parameter). */
    get isInflated() {
      return !!Il2Cpp3.exports.methodIsInflated(this);
    }
    /** Determines whether this method is static. */
    get isStatic() {
      return !Il2Cpp3.exports.methodIsInstance(this);
    }
    /** Determines whether this method is synchronized. */
    get isSynchronized() {
      return (this.implementationFlags & 32) != 0;
    }
    /** Gets the access modifier of this method. */
    get modifier() {
      switch (this.flags & 7) {
        case 1:
          return "private";
        case 2:
          return "private protected";
        case 3:
          return "internal";
        case 4:
          return "protected";
        case 5:
          return "protected internal";
        case 6:
          return "public";
      }
    }
    /** Gets the name of this method. */
    get name() {
      return Il2Cpp3.exports.methodGetName(this).readUtf8String();
    }
    /** @internal */
    get nativeFunction() {
      return new NativeFunction(this.virtualAddress, this.returnType.fridaAlias, this.fridaSignature);
    }
    /** @internal */
    get nativeSignatureHasInstanceSlot() {
      return !this.isStatic || staticMethodsRequireInstanceParam();
    }
    /** Gets the encompassing object of the current method. */
    get object() {
      return new Il2Cpp3.Object(Il2Cpp3.exports.methodGetObject(this, NULL));
    }
    /** Gets the amount of parameters of this method. */
    get parameterCount() {
      return Il2Cpp3.exports.methodGetParameterCount(this);
    }
    /** Gets the parameters of this method. */
    get parameters() {
      return globalThis.Array.from(globalThis.Array(this.parameterCount), (_, i) => {
        const parameterName = Il2Cpp3.exports.methodGetParameterName(this, i).readUtf8String();
        const parameterType = Il2Cpp3.exports.methodGetParameterType(this, i);
        return new Il2Cpp3.Parameter(parameterName, i, new Il2Cpp3.Type(parameterType));
      });
    }
    /** Gets the relative virtual address (RVA) of this method. */
    get relativeVirtualAddress() {
      return this.virtualAddress.sub(Il2Cpp3.module.base);
    }
    /** Gets the return type of this method. */
    get returnType() {
      return new Il2Cpp3.Type(Il2Cpp3.exports.methodGetReturnType(this));
    }
    /** Gets the virtual address (VA) of this method. */
    get virtualAddress() {
      const offset = virtualAddressOffset();
      getter(Il2Cpp3.Method.prototype, "virtualAddress", function() {
        return this.handle.add(offset).readPointer();
      }, lazy);
      Il2Cpp3.corlib.class("System.Reflection.Module").method(".cctor").invoke();
      return this.virtualAddress;
    }
    /** Replaces the body of this method. */
    set implementation(block) {
      try {
        Interceptor.replace(this.virtualAddress, this.wrap(block));
      } catch (e) {
        switch (e.message) {
          case "access violation accessing 0x0":
            raise(`couldn't set implementation for method ${this.name} as it has a NULL virtual address`);
          case /unable to intercept function at \w+; please file a bug/.exec(e.message)?.input:
            warn(`couldn't set implementation for method ${this.name} as it may be a thunk`);
            break;
          case "already replaced this function":
            warn(`couldn't set implementation for method ${this.name} as it has already been replaced by a thunk`);
            break;
          default:
            throw e;
        }
      }
    }
    /** Creates a generic instance of the current generic method. */
    inflate(...classes) {
      if (!this.isGeneric || this.generics.length != classes.length) {
        for (const method of this.overloads()) {
          if (method.isGeneric && method.generics.length == classes.length) {
            return method.inflate(...classes);
          }
        }
        raise(`could not find inflatable signature of method ${this.name} with ${classes.length} generic parameter(s)`);
      }
      const types = classes.map((_) => _.type.object);
      const typeArray = Il2Cpp3.array(Il2Cpp3.corlib.class("System.Type"), types);
      const inflatedMethodObject = this.object.method("MakeGenericMethod", 1).invoke(typeArray);
      return new Il2Cpp3.Method(inflatedMethodObject.field("mhandle").value);
    }
    /** Invokes this method. */
    invoke(...parameters) {
      if (!this.isStatic) {
        raise(`cannot invoke non-static method ${this.name} as it must be invoked throught a Il2Cpp.Object, not a Il2Cpp.Class`);
      }
      return this.invokeRaw(NULL, ...parameters);
    }
    /** @internal */
    invokeRaw(instance, ...parameters) {
      const allocatedParameters = parameters.map(Il2Cpp3.toFridaValue);
      if (this.nativeSignatureHasInstanceSlot) {
        allocatedParameters.unshift(instance);
      }
      if (this.isInflated) {
        allocatedParameters.push(this.handle);
      }
      try {
        const returnValue = this.nativeFunction(...allocatedParameters);
        return Il2Cpp3.fromFridaValue(returnValue, this.returnType);
      } catch (e) {
        if (e == null) {
          raise("an unexpected native invocation exception occurred, this is due to parameter types mismatch");
        }
        switch (e.message) {
          case "bad argument count":
            raise(`couldn't invoke method ${this.name} as it needs ${this.parameterCount} parameter(s), not ${parameters.length}`);
          case "expected a pointer":
          case "expected number":
          case "expected array with fields":
            raise(`couldn't invoke method ${this.name} using incorrect parameter types`);
        }
        throw e;
      }
    }
    /** Gets the overloaded method with the given parameter types. */
    overload(...typeNamesOrClasses) {
      const method = this.tryOverload(...typeNamesOrClasses);
      return method ?? raise(`couldn't find overloaded method ${this.name}(${typeNamesOrClasses.map((_) => _ instanceof Il2Cpp3.Class ? _.type.name : _)})`);
    }
    /** @internal */
    *overloads() {
      for (const klass of this.class.hierarchy()) {
        for (const method of klass.methods) {
          if (this.name == method.name) {
            yield method;
          }
        }
      }
    }
    /** Gets the parameter with the given name. */
    parameter(name) {
      return this.tryParameter(name) ?? raise(`couldn't find parameter ${name} in method ${this.name}`);
    }
    /** Restore the original method implementation. */
    revert() {
      Interceptor.revert(this.virtualAddress);
      Interceptor.flush();
    }
    /** Gets the overloaded method with the given parameter types. */
    tryOverload(...typeNamesOrClasses) {
      const minScore = typeNamesOrClasses.length * 1;
      const maxScore = typeNamesOrClasses.length * 2;
      let candidate = void 0;
      loop: for (const method of this.overloads()) {
        if (method.parameterCount != typeNamesOrClasses.length)
          continue;
        let score = 0;
        let i = 0;
        for (const parameter of method.parameters) {
          const desiredTypeNameOrClass = typeNamesOrClasses[i];
          if (desiredTypeNameOrClass instanceof Il2Cpp3.Class) {
            if (parameter.type.is(desiredTypeNameOrClass.type)) {
              score += 2;
            } else if (parameter.type.class.isAssignableFrom(desiredTypeNameOrClass)) {
              score += 1;
            } else {
              continue loop;
            }
          } else if (parameter.type.name == desiredTypeNameOrClass) {
            score += 2;
          } else {
            continue loop;
          }
          i++;
        }
        if (score < minScore) {
          continue;
        } else if (score == maxScore) {
          return method;
        } else if (candidate == void 0 || score > candidate[0]) {
          candidate = [score, method];
        } else if (score == candidate[0]) {
          let i2 = 0;
          for (const parameter of candidate[1].parameters) {
            if (parameter.type.class.isAssignableFrom(method.parameters[i2].type.class)) {
              candidate = [score, method];
              continue loop;
            }
            i2++;
          }
        }
      }
      return candidate?.[1];
    }
    /** Gets the parameter with the given name. */
    tryParameter(name) {
      return this.parameters.find((_) => _.name == name);
    }
    /** */
    toString() {
      return `${this.isStatic ? `static ` : ``}${this.returnType.name} ${this.name}${this.generics.length > 0 ? `<${this.generics.map((_) => _.type.name).join(",")}>` : ""}(${this.parameters.join(`, `)});${this.virtualAddress.isNull() ? `` : ` // 0x${this.relativeVirtualAddress.toString(16).padStart(8, `0`)}`}`;
    }
    /**
     * @internal
     * Binds the current method to a {@link Il2Cpp.Object} or a
     * {@link Il2Cpp.ValueType} (also known as *instances*), so that it is
     * possible to invoke it - see {@link Il2Cpp.Method.invoke} for
     * details. \
     * Binding a static method is forbidden.
     */
    bind(instance) {
      if (this.isStatic) {
        raise(`cannot bind static method ${this.class.type.name}::${this.name} to an instance`);
      }
      return new Proxy(this, {
        get(target, property, receiver) {
          switch (property) {
            case "invoke":
              const handle = instance instanceof Il2Cpp3.ValueType ? target.class.isValueType ? instance.handle.sub(structMethodsRequireObjectInstances() ? Il2Cpp3.Object.headerSize : 0) : raise(`cannot invoke method ${target.class.type.name}::${target.name} against a value type, you must box it first`) : target.class.isValueType ? instance.handle.add(structMethodsRequireObjectInstances() ? 0 : Il2Cpp3.Object.headerSize) : instance.handle;
              return target.invokeRaw.bind(target, handle);
            case "overloads":
              return function* () {
                for (const method of target[property]()) {
                  if (!method.isStatic) {
                    yield method;
                  }
                }
              };
            case "inflate":
            case "overload":
            case "tryOverload":
              const member = Reflect.get(target, property).bind(receiver);
              return function(...args) {
                return member(...args)?.bind(instance);
              };
          }
          return Reflect.get(target, property);
        }
      });
    }
    /** @internal */
    wrap(block) {
      const parameterStartIndex = +this.nativeSignatureHasInstanceSlot;
      return new NativeCallback((...args) => {
        const thisObject = this.isStatic ? this.class : this.class.isValueType ? new Il2Cpp3.ValueType(args[0].add(structMethodsRequireObjectInstances() ? Il2Cpp3.Object.headerSize : 0), this.class.type) : new Il2Cpp3.Object(args[0]);
        const parameters = this.parameters.map((_, i) => Il2Cpp3.fromFridaValue(args[i + parameterStartIndex], _.type));
        const result = block.call(thisObject, ...parameters);
        return Il2Cpp3.toFridaValue(result);
      }, this.returnType.fridaAlias, this.fridaSignature);
    }
    /**
     * Attempts to find a {@link Il2Cpp.Method} given its virtual address, that is, the native
     * function's first instruction address.
     *
     * Please note that different generic methods (or methods belonging to generic classes) may
     * share the same virtual address due to IL2CPP generic sharing: as a consequence, an
     * unexpected `Il2Cpp.Method` might be returned. However, such shared virtual addresses are
     * not actually invocable, so any address taken from runtime execution (e.g. from tracing)
     * is not affected.
     *
     * To resolve a method from any address within the native function body, use
     * {@link Il2Cpp.Method.fromAnyAddress}.
     * ```ts
     * const virtualAddress: NativePointer = ...;
     * Il2Cpp.Method.fromVirtualAddress(virtualAddress);
     * ```
     */
    static fromVirtualAddress(virtualAddress) {
      return this.tryFromVirtualAddress(virtualAddress) ?? raise(`couldn't find method at virtual address ${virtualAddress}`);
    }
    /**
     * Just like {@link Il2Cpp.Method.fromVirtualAddress}, but returns `undefined` if no method
     * is found.
     * ```ts
     * const virtualAddress: NativePointer = ...;
     * Il2Cpp.Method.tryFromVirtualAddress(virtualAddress);
     * ```
     */
    static tryFromVirtualAddress(virtualAddress) {
      if (virtualAddress.isNull()) {
        raise("method virtual address cannot be null");
      }
      const rangeProvider = function* () {
        yield* rangesHavingMethodDefinitionsCache.values();
        yield* Process.enumerateRanges("rw-").filter((_) => _.file == void 0);
      };
      for (const range of rangeProvider()) {
        let matches;
        try {
          matches = Memory.scanSync(range.base, range.size, virtualAddress.toMatchPattern());
        } catch (_) {
          continue;
        }
        for (const { address } of matches) {
          const method = new Il2Cpp3.Method(address.sub(virtualAddressOffset()));
          try {
            if (method.name == null) {
              continue;
            }
          } catch (_) {
            continue;
          }
          const currentRange = rangesHavingMethodDefinitionsCache.get(range.base.toInt32());
          if (currentRange == void 0 || currentRange.size != range.size) {
            rangesHavingMethodDefinitionsCache.set(range.base.toInt32(), range);
          }
          return method;
        }
      }
    }
    /**
     * Attempts to find a {@link Il2Cpp.Method} given its relative virtual address, that is,
     * the native function's first instruction offset from the IL2CPP module base.
     * ```ts
     * Il2Cpp.Method.fromRelativeVirtualAddress("0x004ac1e");
     * ```
     */
    static fromRelativeVirtualAddress(relativeVirtualAddress) {
      return this.tryFromRelativeVirtualAddress(relativeVirtualAddress) ?? raise(`couldn't find method with RVA 0x${relativeVirtualAddress.toString(16)}`);
    }
    /**
     * Just like {@link Il2Cpp.Method.fromRelativeVirtualAddress}, but returns `undefined`
     * if no method is found.
     * ```ts
     * Il2Cpp.Method.tryFromVirtualAddress("0x004ac1e");
     * ```
     */
    static tryFromRelativeVirtualAddress(relativeVirtualAddress) {
      return this.tryFromVirtualAddress(Il2Cpp3.module.base.add(ptr(relativeVirtualAddress)));
    }
    /**
     * Attempts to find the {@link Il2Cpp.Method} whose native function contains the given
     * address.
     * Unlike {@link Il2Cpp.Method.fromVirtualAddress}, this accepts any address within the
     * native function body, not only its first instruction address.
     *
     * This combines {@link Process.findFunctionRange} and
     * {@link Il2Cpp.Method.fromVirtualAddress}.
     * ```ts
     * const address: NativePointer = ...;
     * Il2Cpp.Method.fromAnyAddress(address);
     * ```
     */
    static fromAnyAddress(address) {
      return this.tryFromAnyAddress(address) ?? raise(`couldn't find method containing address ${address}`);
    }
    /**
     * Just like {@link Il2Cpp.Method.fromAnyAddress}, but returns `undefined` if no method
     * is found.
     * ```ts
     * const address: NativePointer = ...;
     * Il2Cpp.Method.tryFromAnyAddress(address);
     * ```
     */
    static tryFromAnyAddress(address) {
      const range = Process.findFunctionRange(address);
      return range == null ? void 0 : this.tryFromVirtualAddress(range.base);
    }
  }
  __decorate([
    lazy
  ], Method.prototype, "class", null);
  __decorate([
    lazy
  ], Method.prototype, "flags", null);
  __decorate([
    lazy
  ], Method.prototype, "implementationFlags", null);
  __decorate([
    lazy
  ], Method.prototype, "fridaSignature", null);
  __decorate([
    lazy
  ], Method.prototype, "generics", null);
  __decorate([
    lazy
  ], Method.prototype, "isExternal", null);
  __decorate([
    lazy
  ], Method.prototype, "isGeneric", null);
  __decorate([
    lazy
  ], Method.prototype, "isInflated", null);
  __decorate([
    lazy
  ], Method.prototype, "isStatic", null);
  __decorate([
    lazy
  ], Method.prototype, "isSynchronized", null);
  __decorate([
    lazy
  ], Method.prototype, "modifier", null);
  __decorate([
    lazy
  ], Method.prototype, "name", null);
  __decorate([
    lazy
  ], Method.prototype, "nativeFunction", null);
  __decorate([
    lazy
  ], Method.prototype, "object", null);
  __decorate([
    lazy
  ], Method.prototype, "parameterCount", null);
  __decorate([
    lazy
  ], Method.prototype, "parameters", null);
  __decorate([
    lazy
  ], Method.prototype, "relativeVirtualAddress", null);
  __decorate([
    lazy
  ], Method.prototype, "returnType", null);
  Il2Cpp3.Method = Method;
  let staticMethodsRequireInstanceParam = () => {
    const SystemObject = Il2Cpp3.corlib.class("System.Object");
    const Equals = new NativeFunction(SystemObject.method("Equals", 2).virtualAddress, "bool", ["pointer", "pointer"]);
    const sentinel = SystemObject.new();
    const equality = Equals(sentinel, sentinel);
    const result = !equality;
    return (staticMethodsRequireInstanceParam = () => result)();
  };
  let structMethodsRequireObjectInstances = () => {
    const object = Il2Cpp3.corlib.class("System.Int64").alloc();
    object.field("m_value").value = 3735928559;
    const result = object.method("Equals", 1).overload(object.class).invokeRaw(object, 3735928559);
    return (structMethodsRequireObjectInstances = () => result)();
  };
  let virtualAddressOffset = () => {
    const FilterTypeName = Il2Cpp3.corlib.class("System.Reflection.Module").initialize().field("FilterTypeName").value;
    const FilterTypeNameMethodPointer = FilterTypeName.field("method_ptr").value;
    const FilterTypeNameMethod = FilterTypeName.field("method").value;
    const offset = FilterTypeNameMethod.offsetOf((_) => _.readPointer().equals(FilterTypeNameMethodPointer)) ?? raise("couldn't find the virtual address offset in the native method struct");
    return (virtualAddressOffset = () => offset)();
  };
  const rangesHavingMethodDefinitionsCache = /* @__PURE__ */ new Map();
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  class Object2 extends NativeStruct {
    /** Gets the Il2CppObject struct size, possibly equal to `Process.pointerSize * 2`. */
    static get headerSize() {
      return Il2Cpp3.corlib.class("System.Object").instanceSize;
    }
    /**
     * Returns the same object, but having its parent class as class.
     * It basically is the C# `base` keyword, so that parent members can be
     * accessed.
     *
     * **Example** \
     * Consider the following classes:
     * ```csharp
     * class Foo
     * {
     *     int foo()
     *     {
     *          return 1;
     *     }
     * }
     * class Bar : Foo
     * {
     *     new int foo()
     *     {
     *          return 2;
     *     }
     * }
     * ```
     * then:
     * ```ts
     * const Bar: Il2Cpp.Class = ...;
     * const bar = Bar.new();
     *
     * console.log(bar.foo()); // 2
     * console.log(bar.base.foo()); // 1
     * ```
     */
    get base() {
      if (this.class.parent == null) {
        raise(`class ${this.class.type.name} has no parent`);
      }
      return new Proxy(this, {
        get(target, property, receiver) {
          if (property == "class") {
            return Reflect.get(target, property).parent;
          } else if (property == "base") {
            return Reflect.getOwnPropertyDescriptor(Il2Cpp3.Object.prototype, property).get.bind(receiver)();
          }
          return Reflect.get(target, property);
        }
      });
    }
    /** Gets the class of this object. */
    get class() {
      return new Il2Cpp3.Class(Il2Cpp3.exports.objectGetClass(this));
    }
    /** Returns a monitor for this object. */
    get monitor() {
      return new Il2Cpp3.Object.Monitor(this);
    }
    /** Gets the size of the current object. */
    get size() {
      return Il2Cpp3.exports.objectGetSize(this);
    }
    /** Gets the non-static field with the given name of the current class hierarchy. */
    field(name) {
      return this.tryField(name) ?? raise(`couldn't find non-static field ${name} in hierarchy of class ${this.class.type.name}`);
    }
    /** Gets the non-static method with the given name (and optionally parameter count) of the current class hierarchy. */
    method(name, parameterCount = -1) {
      return this.tryMethod(name, parameterCount) ?? raise(`couldn't find non-static method ${name} in hierarchy of class ${this.class.type.name}`);
    }
    /** Creates a reference to this object. */
    ref(pin) {
      return new Il2Cpp3.GCHandle(Il2Cpp3.exports.gcHandleNew(this, +pin));
    }
    /** Gets the correct virtual method from the given virtual method. */
    virtualMethod(method) {
      return new Il2Cpp3.Method(Il2Cpp3.exports.objectGetVirtualMethod(this, method)).bind(this);
    }
    /** Gets the non-static field with the given name of the current class hierarchy, if it exists. */
    tryField(name) {
      const field = this.class.tryField(name);
      if (field?.isStatic) {
        for (const klass of this.class.hierarchy({ includeCurrent: false })) {
          for (const field2 of klass.fields) {
            if (field2.name == name && !field2.isStatic) {
              return field2.bind(this);
            }
          }
        }
        return void 0;
      }
      return field?.bind(this);
    }
    /** Gets the non-static method with the given name (and optionally parameter count) of the current class hierarchy, if it exists. */
    tryMethod(name, parameterCount = -1) {
      const method = this.class.tryMethod(name, parameterCount);
      if (method?.isStatic) {
        for (const klass of this.class.hierarchy()) {
          for (const method2 of klass.methods) {
            if (method2.name == name && !method2.isStatic && (parameterCount < 0 || method2.parameterCount == parameterCount)) {
              return method2.bind(this);
            }
          }
        }
        return void 0;
      }
      return method?.bind(this);
    }
    /** */
    toString() {
      return this.isNull() ? "null" : this.method("ToString", 0).invoke().content ?? "null";
    }
    /** Unboxes the value type (either a primitive, a struct or an enum) out of this object. */
    unbox() {
      return this.class.isValueType ? new Il2Cpp3.ValueType(Il2Cpp3.exports.objectUnbox(this), this.class.type) : raise(`couldn't unbox instances of ${this.class.type.name} as they are not value types`);
    }
    /** Creates a weak reference to this object. */
    weakRef(trackResurrection) {
      return new Il2Cpp3.GCHandle(Il2Cpp3.exports.gcHandleNewWeakRef(this, +trackResurrection));
    }
  }
  __decorate([
    lazy
  ], Object2.prototype, "class", null);
  __decorate([
    lazy
  ], Object2.prototype, "size", null);
  __decorate([
    lazy
  ], Object2, "headerSize", null);
  Il2Cpp3.Object = Object2;
  (function(Object3) {
    class Monitor {
      handle;
      /** @internal */
      constructor(handle) {
        this.handle = handle;
      }
      /** Acquires an exclusive lock on the current object. */
      enter() {
        return Il2Cpp3.exports.monitorEnter(this.handle);
      }
      /** Release an exclusive lock on the current object. */
      exit() {
        return Il2Cpp3.exports.monitorExit(this.handle);
      }
      /** Notifies a thread in the waiting queue of a change in the locked object's state. */
      pulse() {
        return Il2Cpp3.exports.monitorPulse(this.handle);
      }
      /** Notifies all waiting threads of a change in the object's state. */
      pulseAll() {
        return Il2Cpp3.exports.monitorPulseAll(this.handle);
      }
      /** Attempts to acquire an exclusive lock on the current object. */
      tryEnter(timeout) {
        return !!Il2Cpp3.exports.monitorTryEnter(this.handle, timeout);
      }
      /** Releases the lock on an object and attempts to block the current thread until it reacquires the lock. */
      tryWait(timeout) {
        return !!Il2Cpp3.exports.monitorTryWait(this.handle, timeout);
      }
      /** Releases the lock on an object and blocks the current thread until it reacquires the lock. */
      wait() {
        return Il2Cpp3.exports.monitorWait(this.handle);
      }
    }
    Object3.Monitor = Monitor;
  })(Object2 = Il2Cpp3.Object || (Il2Cpp3.Object = {}));
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  class Parameter {
    /** Name of this parameter. */
    name;
    /** Position of this parameter. */
    position;
    /** Type of this parameter. */
    type;
    constructor(name, position, type) {
      this.name = name;
      this.position = position;
      this.type = type;
    }
    /** */
    toString() {
      return `${this.type.name} ${this.name}`;
    }
  }
  Il2Cpp3.Parameter = Parameter;
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  class Pointer extends NativeStruct {
    type;
    constructor(handle, type) {
      super(handle);
      this.type = type;
    }
    /** Gets the element at the given index. */
    get(index) {
      return Il2Cpp3.read(this.handle.add(index * this.type.class.arrayElementSize), this.type);
    }
    /** Reads the given amount of elements starting at the given offset. */
    read(length, offset = 0) {
      const values = new globalThis.Array(length);
      for (let i = 0; i < length; i++) {
        values[i] = this.get(i + offset);
      }
      return values;
    }
    /** Sets the given element at the given index */
    set(index, value) {
      Il2Cpp3.write(this.handle.add(index * this.type.class.arrayElementSize), value, this.type);
    }
    /** */
    toString() {
      return this.handle.toString();
    }
    /** Writes the given elements starting at the given index. */
    write(values, offset = 0) {
      for (let i = 0; i < values.length; i++) {
        this.set(i + offset, values[i]);
      }
    }
  }
  Il2Cpp3.Pointer = Pointer;
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  class Reference extends NativeStruct {
    type;
    constructor(handle, type) {
      super(handle);
      this.type = type;
    }
    /** Gets the element referenced by the current reference. */
    get value() {
      return Il2Cpp3.read(this.handle, this.type);
    }
    /** Sets the element referenced by the current reference. */
    set value(value) {
      Il2Cpp3.write(this.handle, value, this.type);
    }
    /** */
    toString() {
      return this.isNull() ? "null" : `->${this.value}`;
    }
  }
  Il2Cpp3.Reference = Reference;
  function reference(value, type) {
    const handle = Memory.alloc(Process.pointerSize);
    switch (typeof value) {
      case "boolean":
        return new Il2Cpp3.Reference(handle.writeS8(+value), Il2Cpp3.corlib.class("System.Boolean").type);
      case "number":
        switch (type?.enumValue) {
          case Il2Cpp3.Type.Enum.UBYTE:
            return new Il2Cpp3.Reference(handle.writeU8(value), type);
          case Il2Cpp3.Type.Enum.BYTE:
            return new Il2Cpp3.Reference(handle.writeS8(value), type);
          case Il2Cpp3.Type.Enum.CHAR:
          case Il2Cpp3.Type.Enum.USHORT:
            return new Il2Cpp3.Reference(handle.writeU16(value), type);
          case Il2Cpp3.Type.Enum.SHORT:
            return new Il2Cpp3.Reference(handle.writeS16(value), type);
          case Il2Cpp3.Type.Enum.UINT:
            return new Il2Cpp3.Reference(handle.writeU32(value), type);
          case Il2Cpp3.Type.Enum.INT:
            return new Il2Cpp3.Reference(handle.writeS32(value), type);
          case Il2Cpp3.Type.Enum.ULONG:
            return new Il2Cpp3.Reference(handle.writeU64(value), type);
          case Il2Cpp3.Type.Enum.LONG:
            return new Il2Cpp3.Reference(handle.writeS64(value), type);
          case Il2Cpp3.Type.Enum.FLOAT:
            return new Il2Cpp3.Reference(handle.writeFloat(value), type);
          case Il2Cpp3.Type.Enum.DOUBLE:
            return new Il2Cpp3.Reference(handle.writeDouble(value), type);
        }
      case "object":
        if (value instanceof Il2Cpp3.ValueType || value instanceof Il2Cpp3.Pointer) {
          return new Il2Cpp3.Reference(value.handle, value.type);
        } else if (value instanceof Il2Cpp3.Object) {
          return new Il2Cpp3.Reference(handle.writePointer(value), value.class.type);
        } else if (value instanceof Il2Cpp3.String || value instanceof Il2Cpp3.Array) {
          return new Il2Cpp3.Reference(handle.writePointer(value), value.object.class.type);
        } else if (value instanceof NativePointer) {
          switch (type?.enumValue) {
            case Il2Cpp3.Type.Enum.NUINT:
            case Il2Cpp3.Type.Enum.NINT:
              return new Il2Cpp3.Reference(handle.writePointer(value), type);
          }
        } else if (value instanceof Int64) {
          return new Il2Cpp3.Reference(handle.writeS64(value), Il2Cpp3.corlib.class("System.Int64").type);
        } else if (value instanceof UInt64) {
          return new Il2Cpp3.Reference(handle.writeU64(value), Il2Cpp3.corlib.class("System.UInt64").type);
        }
      default:
        raise(`couldn't create a reference to ${value} using an unhandled type ${type?.name}`);
    }
  }
  Il2Cpp3.reference = reference;
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  class String extends NativeStruct {
    /** Gets the content of this string. */
    get content() {
      return Il2Cpp3.exports.stringGetChars(this).readUtf16String(this.length);
    }
    /** @unsafe Sets the content of this string - it may write out of bounds! */
    set content(value) {
      const offset = Il2Cpp3.string("vfsfitvnm").handle.offsetOf((_) => _.readInt() == 9) ?? raise("couldn't find the length offset in the native string struct");
      globalThis.Object.defineProperty(Il2Cpp3.String.prototype, "content", {
        set(value2) {
          Il2Cpp3.exports.stringGetChars(this).writeUtf16String(value2 ?? "");
          this.handle.add(offset).writeS32(value2?.length ?? 0);
        }
      });
      this.content = value;
    }
    /** Gets the length of this string. */
    get length() {
      return Il2Cpp3.exports.stringGetLength(this);
    }
    /** Gets the encompassing object of the current string. */
    get object() {
      return new Il2Cpp3.Object(this);
    }
    /** */
    toString() {
      return this.isNull() ? "null" : `"${this.content}"`;
    }
  }
  Il2Cpp3.String = String;
  function string(content) {
    return new Il2Cpp3.String(Il2Cpp3.exports.stringNew(Memory.allocUtf8String(content ?? "")));
  }
  Il2Cpp3.string = string;
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  class Thread extends NativeStruct {
    /**
     * Gets the managed thread associated with the given native thread id (if any).
     *
     * This is an alternative to {@link Il2Cpp.attachedThreads}:
     * ```ts
     * async function findAllThreads() {
     *     const promises = Process.enumerateThreads().map(thread => Il2Cpp.Thread.fromId(thread.id));
     *     return (await Promise.all(promises)).filter(thread => thread != null);
     * }
     * ```
     */
    static fromId(threadId) {
      return Process.runOnThread(threadId, () => Il2Cpp3.currentThread);
    }
    /**
     * Gets the native (operative system) thread id of this thread, or a negative number if
     * it's not available.
     */
    get id() {
      let get = function() {
        return this.internal.field("thread_id").value.toNumber();
      };
      if (Process.platform != "windows") {
        const currentThreadId = Process.getCurrentThreadId();
        const currentPosixThread = ptr(get.apply(Il2Cpp3.currentThread));
        const offset = currentPosixThread.offsetOf((_) => _.readS32() == currentThreadId, 1024) ?? raise(`couldn't find the offset for determining the kernel id of a posix thread`);
        const getPosixThreadHandle = get;
        get = function() {
          const handle = ptr(getPosixThreadHandle.apply(this));
          if (handle.isNull()) {
            warn(`couldn't find thread id for ${this.handle}; returning a negative number, expect breakage`);
            return -1;
          }
          return handle.add(offset).readS32();
        };
      }
      getter(Il2Cpp3.Thread.prototype, "id", get, lazy);
      return this.id;
    }
    /**
     * Gets the internal managed thread (`System.Threading.InternalThread`) of this thread - actually a
     * [`MonoInternalThread`](https://github.com/mono/mono/blob/0f53e9e151d92944cacab3e24ac359410c606df6/mono/metadata/object-internals.h#L575).
     */
    get internal() {
      return this.object.tryField("internal_thread")?.value ?? // in older Unity versions, `System.Threading.Thread` already was a `MonoInternalThread`
      this.object;
    }
    /** Determines whether the current thread is the garbage collector finalizer one. */
    get isFinalizer() {
      return !Il2Cpp3.exports.threadIsVm(this);
    }
    /** Gets the managed id of the current thread. */
    get managedId() {
      return this.object.method("get_ManagedThreadId").invoke();
    }
    /** Gets the encompassing object of the current thread. */
    get object() {
      return new Il2Cpp3.Object(this);
    }
    /** @internal */
    get staticData() {
      return this.internal.field("static_data").value;
    }
    /** @internal */
    get synchronizationContext() {
      const get_ExecutionContext = this.object.tryMethod("GetMutableExecutionContext") ?? this.object.method("get_ExecutionContext");
      const executionContext = get_ExecutionContext.invoke();
      const synchronizationContext = executionContext.tryField("_syncContext")?.value ?? executionContext.tryMethod("get_SynchronizationContext")?.invoke() ?? this.tryLocalValue(Il2Cpp3.corlib.class("System.Threading.SynchronizationContext"));
      return synchronizationContext?.asNullable() ?? null;
    }
    /** Detaches the thread from the application domain. */
    detach() {
      return Il2Cpp3.exports.threadDetach(this);
    }
    /** Schedules a callback on the current thread. */
    schedule(block) {
      const Post = this.synchronizationContext?.tryMethod("Post");
      if (Post == null) {
        return Process.runOnThread(this.id, block);
      }
      return new Promise((resolve) => {
        const delegate = Il2Cpp3.delegate(Il2Cpp3.corlib.class("System.Threading.SendOrPostCallback"), () => {
          const result = block();
          setImmediate(() => resolve(result));
        });
        Script.bindWeak(globalThis, () => {
          delegate.field("method_ptr").value = delegate.field("invoke_impl").value = Il2Cpp3.exports.domainGet;
        });
        Post.invoke(delegate, NULL);
      });
    }
    /** @internal */
    tryLocalValue(klass) {
      for (let i = 0; i < 16; i++) {
        const base = this.staticData.add(i * Process.pointerSize).readPointer();
        if (!base.isNull()) {
          const object = new Il2Cpp3.Object(base.readPointer()).asNullable();
          if (object?.class?.isSubclassOf(klass, false)) {
            return object;
          }
        }
      }
    }
  }
  __decorate([
    lazy
  ], Thread.prototype, "internal", null);
  __decorate([
    lazy
  ], Thread.prototype, "isFinalizer", null);
  __decorate([
    lazy
  ], Thread.prototype, "managedId", null);
  __decorate([
    lazy
  ], Thread.prototype, "object", null);
  __decorate([
    lazy
  ], Thread.prototype, "staticData", null);
  __decorate([
    lazy
  ], Thread.prototype, "synchronizationContext", null);
  Il2Cpp3.Thread = Thread;
  getter(Il2Cpp3, "attachedThreads", () => {
    if (Il2Cpp3.exports.threadGetAttachedThreads.isNull()) {
      const currentThread = Il2Cpp3.currentThread ?? raise("current thread is not attached to IL2CPP");
      const pattern = currentThread.handle.toMatchPattern();
      const rangeProvider = function* () {
        try {
          yield* Il2Cpp3.gc.heapSections;
        } catch (_) {
        }
        yield Process.getRangeByAddress(Il2Cpp3.corlib.assembly.object);
        yield Process.getRangeByAddress(Il2Cpp3.corlib.class("System.Threading.Thread").staticFieldsData);
        yield* Process.enumerateRanges("rw-").filter((_) => _.file == void 0);
      };
      for (const range of rangeProvider()) {
        let matches = [];
        try {
          matches = Memory.scanSync(range.base, range.size, pattern);
        } catch (_) {
        }
        if (matches.length == 1) {
          const threads = [];
          const iter = (index) => {
            const handle = matches[0].address.add(index * Process.pointerSize).readPointer();
            if (!handle.isNull() && Process.findRangeByAddress(handle) != null) {
              const thread = new Il2Cpp3.Thread(handle);
              if (thread.object.class.equals(currentThread.object.class)) {
                return threads.push(thread);
              }
            }
          };
          for (let i = 0; iter(i); i--)
            ;
          threads.reverse();
          for (let i = 1; iter(i); i++)
            ;
          return threads;
        }
      }
      raise("couldn't collect attached threads");
    }
    return readNativeList(Il2Cpp3.exports.threadGetAttachedThreads).map((_) => new Il2Cpp3.Thread(_));
  });
  getter(Il2Cpp3, "currentThread", () => {
    return new Il2Cpp3.Thread(Il2Cpp3.exports.threadGetCurrent()).asNullable();
  });
  getter(Il2Cpp3, "mainThread", () => {
    return Il2Cpp3.attachedThreads[0];
  });
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  let Type = class Type extends NativeStruct {
    /** */
    static get Enum() {
      const _ = (_2, block = (_3) => _3) => block(Il2Cpp3.corlib.class(_2)).type.enumValue;
      const initial = {
        VOID: _("System.Void"),
        BOOLEAN: _("System.Boolean"),
        CHAR: _("System.Char"),
        BYTE: _("System.SByte"),
        UBYTE: _("System.Byte"),
        SHORT: _("System.Int16"),
        USHORT: _("System.UInt16"),
        INT: _("System.Int32"),
        UINT: _("System.UInt32"),
        LONG: _("System.Int64"),
        ULONG: _("System.UInt64"),
        NINT: _("System.IntPtr"),
        NUINT: _("System.UIntPtr"),
        FLOAT: _("System.Single"),
        DOUBLE: _("System.Double"),
        POINTER: _("System.IntPtr", (_2) => _2.field("m_value")),
        VALUE_TYPE: _("System.Decimal"),
        OBJECT: _("System.Object"),
        STRING: _("System.String"),
        CLASS: _("System.Array"),
        ARRAY: _("System.Void", (_2) => _2.arrayClass),
        NARRAY: _("System.Void", (_2) => new Il2Cpp3.Class(Il2Cpp3.exports.classGetArrayClass(_2, 2))),
        GENERIC_INSTANCE: _("System.Int32", (_2) => _2.interfaces.find((_3) => _3.name.endsWith("`1")))
      };
      Reflect.defineProperty(this, "Enum", { value: initial });
      return addFlippedEntries({
        ...initial,
        VAR: _("System.Action`1", (_2) => _2.generics[0]),
        MVAR: _("System.Array", (_2) => _2.method("AsReadOnly", 1).generics[0])
      });
    }
    /** Gets the class of this type. */
    get class() {
      return new Il2Cpp3.Class(Il2Cpp3.exports.typeGetClass(this));
    }
    /** */
    get fridaAlias() {
      function getValueTypeFields(type) {
        const instanceFields = type.class.fields.filter((_) => !_.isStatic);
        return instanceFields.length == 0 ? ["char"] : instanceFields.map((_) => _.type.fridaAlias);
      }
      if (this.isByReference) {
        return "pointer";
      }
      switch (this.enumValue) {
        case Il2Cpp3.Type.Enum.VOID:
          return "void";
        case Il2Cpp3.Type.Enum.BOOLEAN:
          return "bool";
        case Il2Cpp3.Type.Enum.CHAR:
          return "uchar";
        case Il2Cpp3.Type.Enum.BYTE:
          return "int8";
        case Il2Cpp3.Type.Enum.UBYTE:
          return "uint8";
        case Il2Cpp3.Type.Enum.SHORT:
          return "int16";
        case Il2Cpp3.Type.Enum.USHORT:
          return "uint16";
        case Il2Cpp3.Type.Enum.INT:
          return "int32";
        case Il2Cpp3.Type.Enum.UINT:
          return "uint32";
        case Il2Cpp3.Type.Enum.LONG:
          return "int64";
        case Il2Cpp3.Type.Enum.ULONG:
          return "uint64";
        case Il2Cpp3.Type.Enum.FLOAT:
          return "float";
        case Il2Cpp3.Type.Enum.DOUBLE:
          return "double";
        case Il2Cpp3.Type.Enum.NINT:
        case Il2Cpp3.Type.Enum.NUINT:
        case Il2Cpp3.Type.Enum.POINTER:
        case Il2Cpp3.Type.Enum.STRING:
        case Il2Cpp3.Type.Enum.ARRAY:
        case Il2Cpp3.Type.Enum.NARRAY:
          return "pointer";
        case Il2Cpp3.Type.Enum.VALUE_TYPE:
          return this.class.isEnum ? this.class.baseType.fridaAlias : getValueTypeFields(this);
        case Il2Cpp3.Type.Enum.CLASS:
        case Il2Cpp3.Type.Enum.OBJECT:
        case Il2Cpp3.Type.Enum.GENERIC_INSTANCE:
          return this.class.isStruct ? getValueTypeFields(this) : this.class.isEnum ? this.class.baseType.fridaAlias : "pointer";
        default:
          return "pointer";
      }
    }
    /** Determines whether this type is passed by reference. */
    get isByReference() {
      return this.name.endsWith("&");
    }
    /** Determines whether this type is primitive. */
    get isPrimitive() {
      switch (this.enumValue) {
        case Il2Cpp3.Type.Enum.BOOLEAN:
        case Il2Cpp3.Type.Enum.CHAR:
        case Il2Cpp3.Type.Enum.BYTE:
        case Il2Cpp3.Type.Enum.UBYTE:
        case Il2Cpp3.Type.Enum.SHORT:
        case Il2Cpp3.Type.Enum.USHORT:
        case Il2Cpp3.Type.Enum.INT:
        case Il2Cpp3.Type.Enum.UINT:
        case Il2Cpp3.Type.Enum.LONG:
        case Il2Cpp3.Type.Enum.ULONG:
        case Il2Cpp3.Type.Enum.FLOAT:
        case Il2Cpp3.Type.Enum.DOUBLE:
        case Il2Cpp3.Type.Enum.NINT:
        case Il2Cpp3.Type.Enum.NUINT:
          return true;
        default:
          return false;
      }
    }
    /** Gets the name of this type. */
    get name() {
      const handle = Il2Cpp3.exports.typeGetName(this);
      try {
        return handle.readUtf8String();
      } finally {
        Il2Cpp3.free(handle);
      }
    }
    /** Gets the encompassing object of the current type. */
    get object() {
      return new Il2Cpp3.Object(Il2Cpp3.exports.typeGetObject(this));
    }
    /** Gets the {@link Il2Cpp.Type.Enum} value of the current type. */
    get enumValue() {
      return Il2Cpp3.exports.typeGetTypeEnum(this);
    }
    is(other) {
      if (Il2Cpp3.exports.typeEquals.isNull()) {
        return this.object.method("Equals").invoke(other.object);
      }
      return !!Il2Cpp3.exports.typeEquals(this, other);
    }
    /** */
    toString() {
      return this.name;
    }
  };
  __decorate([
    lazy
  ], Type.prototype, "class", null);
  __decorate([
    lazy
  ], Type.prototype, "fridaAlias", null);
  __decorate([
    lazy
  ], Type.prototype, "isByReference", null);
  __decorate([
    lazy
  ], Type.prototype, "isPrimitive", null);
  __decorate([
    lazy
  ], Type.prototype, "name", null);
  __decorate([
    lazy
  ], Type.prototype, "object", null);
  __decorate([
    lazy
  ], Type.prototype, "enumValue", null);
  __decorate([
    lazy
  ], Type, "Enum", null);
  Type = __decorate([
    recycle
  ], Type);
  Il2Cpp3.Type = Type;
})(Il2Cpp2 || (Il2Cpp2 = {}));
var Il2Cpp2;
(function(Il2Cpp3) {
  class ValueType extends NativeStruct {
    type;
    constructor(handle, type) {
      super(handle);
      this.type = type;
    }
    /** Boxes the current value type in a object. */
    box() {
      return new Il2Cpp3.Object(Il2Cpp3.exports.valueTypeBox(this.type.class, this));
    }
    /** Gets the non-static field with the given name of the current class hierarchy. */
    field(name) {
      return this.tryField(name) ?? raise(`couldn't find non-static field ${name} in hierarchy of class ${this.type.name}`);
    }
    /** Gets the non-static method with the given name (and optionally parameter count) of the current class hierarchy. */
    method(name, parameterCount = -1) {
      return this.tryMethod(name, parameterCount) ?? raise(`couldn't find non-static method ${name} in hierarchy of class ${this.type.name}`);
    }
    /** Gets the non-static field with the given name of the current class hierarchy, if it exists. */
    tryField(name) {
      const field = this.type.class.tryField(name);
      if (field?.isStatic) {
        for (const klass of this.type.class.hierarchy()) {
          for (const field2 of klass.fields) {
            if (field2.name == name && !field2.isStatic) {
              return field2.bind(this);
            }
          }
        }
        return void 0;
      }
      return field?.bind(this);
    }
    /** Gets the non-static method with the given name (and optionally parameter count) of the current class hierarchy, if it exists. */
    tryMethod(name, parameterCount = -1) {
      const method = this.type.class.tryMethod(name, parameterCount);
      if (method?.isStatic) {
        for (const klass of this.type.class.hierarchy()) {
          for (const method2 of klass.methods) {
            if (method2.name == name && !method2.isStatic && (parameterCount < 0 || method2.parameterCount == parameterCount)) {
              return method2.bind(this);
            }
          }
        }
        return void 0;
      }
      return method?.bind(this);
    }
    /** */
    toString() {
      const ToString = this.method("ToString", 0);
      return this.isNull() ? "null" : (
        // If ToString is defined within a value type class, we can
        // avoid a boxing operation.
        ToString.class.isValueType ? ToString.invoke().content ?? "null" : this.box().toString() ?? "null"
      );
    }
  }
  Il2Cpp3.ValueType = ValueType;
})(Il2Cpp2 || (Il2Cpp2 = {}));
globalThis.Il2Cpp = Il2Cpp2;

// src/menu.ts
var AK_LOG = "/data/data/com.JamesWest.NoomiClone/.cache/menu.log";
function akLog(s) {
  const line = "[AK] " + s;
  try {
    console.log(line);
  } catch (_) {
  }
  try {
    const f = new File(AK_LOG, "a");
    f.write(line + "\n");
    f.flush();
    f.close();
  } catch (_) {
  }
}
function akExport(mod, name) {
  try {
    return Process.getModuleByName(mod).getExportByName(name);
  } catch {
    return null;
  }
}
(function antiKill() {
  try {
    akLog("preamble start, pid=" + Process.id);
    const myPid = Process.id;
    const voidExit = [
      ["exit", "void", ["int"]],
      ["_exit", "void", ["int"]],
      ["_Exit", "void", ["int"]],
      ["quick_exit", "void", ["int"]],
      ["exit_group", "void", ["int"]]
    ];
    for (const [name, ret, args] of voidExit) {
      const p = akExport("libc.so", name);
      if (p) {
        try {
          Interceptor.replace(p, new NativeCallback(function() {
            akLog("blocked " + name);
          }, ret, args));
        } catch (e) {
          akLog("replace " + name + " fail: " + e);
        }
      }
    }
    const abortP = akExport("libc.so", "abort");
    if (abortP) {
      try {
        Interceptor.replace(abortP, new NativeCallback(function() {
          akLog("blocked abort");
        }, "void", []));
      } catch (e) {
        akLog("replace abort fail: " + e);
      }
    }
    const kill = akExport("libc.so", "kill");
    if (kill)
      Interceptor.attach(kill, { onEnter(a) {
        const pid = a[0].toInt32();
        if (pid === myPid || pid === 0 || pid === -1) {
          akLog("neutralize kill sig to self");
          a[1] = ptr(0);
        }
      } });
    const tgkill = akExport("libc.so", "tgkill");
    if (tgkill)
      Interceptor.attach(tgkill, { onEnter(a) {
        const tgid = a[0].toInt32();
        if (tgid === myPid) {
          akLog("neutralize tgkill to self");
          a[2] = ptr(0);
        }
      } });
    const pthread_kill = akExport("libc.so", "pthread_kill");
    if (pthread_kill)
      Interceptor.attach(pthread_kill, { onEnter(a) {
        akLog("neutralize pthread_kill");
        a[1] = ptr(0);
      } });
    const raiseP = akExport("libc.so", "raise");
    if (raiseP) {
      try {
        Interceptor.replace(raiseP, new NativeCallback(function() {
          akLog("blocked raise");
          return 0;
        }, "int", ["int"]));
      } catch (_) {
      }
    }
    try {
      Java.perform(() => {
        try {
          const S = Java.use("java.lang.System");
          S.exit.implementation = function(c) {
            akLog("blocked System.exit " + c);
          };
        } catch (_) {
        }
        try {
          const R = Java.use("java.lang.Runtime");
          R.exit.implementation = function(c) {
            akLog("blocked Runtime.exit");
          };
          R.halt.implementation = function(c) {
            akLog("blocked Runtime.halt");
          };
        } catch (_) {
        }
        try {
          const P = Java.use("android.os.Process");
          const myJavaPid = P.myPid();
          P.killProcess.implementation = function(pid) {
            if (pid === myJavaPid) {
              akLog("blocked killProcess self");
              return;
            }
            return this.killProcess(pid);
          };
        } catch (_) {
        }
      });
    } catch (e) {
      akLog("java hooks fail: " + e);
    }
    akLog("preamble installed OK");
  } catch (e) {
    akLog("preamble fatal: " + e);
  }
})();
var CONFIG = {
  // money:     { image: "Assembly-CSharp.dll", klass: "GameData", field: "coins", static: true },
  // intercept: { image: "Assembly-CSharp.dll", klass: "Player",   field: "grips", static: false },
};
var RE_MONEY = /money|coin|cash|gold|gem|crystal|currency|balance|wallet|credit|monet|zolot|den[gь]/i;
var RE_COUNT = /intercept|perehvat|perexvat|grip|regrip|combo|reps|pullup|turnik/i;
var NUMERIC = /* @__PURE__ */ new Set([
  "System.Int32",
  "System.Int64",
  "System.Single",
  "System.Double",
  "System.UInt32",
  "System.UInt64",
  "System.Int16",
  "System.UInt16",
  "System.Byte"
]);
var SKIP_ASM = /^(mscorlib|System|Unity|netstandard|Mono\.|Microsoft|I18N|Newtonsoft)/i;
var DUMP_PATH = "/data/data/com.JamesWest.NoomiClone/noomi_dump.cs";
var flyOn = false;
var flyTimer = null;
var toast = (s) => console.log("[toast]", s);
var log = (s) => console.log("[menu]", s);
function img(name) {
  try {
    const a = Il2Cpp.domain.tryAssembly(name);
    return a ? a.image : null;
  } catch {
    return null;
  }
}
function cls(image, klass) {
  const i = img(image);
  return i ? i.tryClass(klass) : null;
}
function coerce(typeName, raw) {
  const n = Number(raw);
  if (typeName === "System.Int64" || typeName === "System.UInt64")
    return int64(raw);
  if (typeName === "System.Boolean")
    return !!n;
  return n;
}
function gameImages() {
  const out = [];
  for (const asm of Il2Cpp.domain.assemblies) {
    try {
      const i = asm.image;
      if (!SKIP_ASM.test(i.name))
        out.push(i);
    } catch (_) {
    }
  }
  return out;
}
function setBySpec(spec, raw) {
  const klass = cls(spec.image, spec.klass);
  if (!klass)
    throw new Error(`\u043A\u043B\u0430\u0441\u0441 \u043D\u0435 \u043D\u0430\u0439\u0434\u0435\u043D: ${spec.image}/${spec.klass}`);
  const field = klass.field(spec.field);
  const tn = field.type.name;
  const val = coerce(tn, raw);
  let changed = 0;
  if (spec.static || field.isStatic) {
    klass.initialize();
    field.value = val;
    changed++;
    log(`static ${spec.klass}.${spec.field} = ${raw}`);
  } else {
    for (const obj of Il2Cpp.gc.choose(klass)) {
      try {
        obj.field(spec.field).value = val;
        changed++;
      } catch (_) {
      }
    }
    log(`inst ${spec.klass}.${spec.field} = ${raw} (\u043E\u0431\u044A\u0435\u043A\u0442\u043E\u0432: ${changed})`);
  }
  return changed;
}
function setByHeuristic(re, raw) {
  let changed = 0, touched = 0;
  for (const image of gameImages()) {
    for (const klass of image.classes) {
      let fields;
      try {
        fields = klass.fields;
      } catch {
        continue;
      }
      for (const f of fields) {
        if (!re.test(f.name))
          continue;
        let tn = "";
        try {
          tn = f.type.name;
        } catch {
          continue;
        }
        if (!NUMERIC.has(tn))
          continue;
        const val = coerce(tn, raw);
        try {
          if (f.isStatic) {
            klass.initialize();
            f.value = val;
            changed++;
            log(`[\u044D\u0432\u0440] static ${klass.type.name}.${f.name} := ${raw}`);
          } else {
            for (const obj of Il2Cpp.gc.choose(klass)) {
              try {
                obj.field(f.name).value = val;
                changed++;
              } catch (_) {
              }
            }
            log(`[\u044D\u0432\u0440] inst ${klass.type.name}.${f.name} := ${raw}`);
          }
          if (++touched > 60) {
            log("[\u044D\u0432\u0440] \u043E\u0431\u0440\u0435\u0437\u0430\u043D\u043E (>60)");
            return changed;
          }
        } catch (_) {
        }
      }
    }
  }
  return changed;
}
function setMoney(raw) {
  try {
    const n = CONFIG.money ? setBySpec(CONFIG.money, raw) : setByHeuristic(RE_MONEY, raw);
    toast(`\u0414\u0435\u043D\u044C\u0433\u0438: \u043F\u043E\u043B\u0435\u0439 ${n}`);
  } catch (e) {
    toast("\u041E\u0448\u0438\u0431\u043A\u0430 \u0434\u0435\u043D\u0435\u0433: " + e);
  }
}
function setIntercept(raw) {
  try {
    const n = CONFIG.intercept ? setBySpec(CONFIG.intercept, raw) : setByHeuristic(RE_COUNT, raw);
    toast(`\u041F\u0435\u0440\u0435\u0445\u0432\u0430\u0442\u044B: \u043F\u043E\u043B\u0435\u0439 ${n}`);
  } catch (e) {
    toast("\u041E\u0448\u0438\u0431\u043A\u0430 \u043F\u0435\u0440\u0435\u0445\u0432\u0430\u0442\u043E\u0432: " + e);
  }
}
function applyGravity(y) {
  try {
    const Physics = cls("UnityEngine.PhysicsModule.dll", "UnityEngine.Physics");
    if (!Physics)
      return;
    const g = Physics.method("get_gravity").invoke();
    g.field("y").value = y;
    Physics.method("set_gravity").invoke(g);
  } catch (e) {
    log("gravity: " + e);
  }
}
function rigidbodiesGravity(enabled) {
  let n = 0;
  const Rb = cls("UnityEngine.PhysicsModule.dll", "UnityEngine.Rigidbody");
  if (!Rb)
    return 0;
  for (const obj of Il2Cpp.gc.choose(Rb)) {
    try {
      obj.method("set_useGravity").invoke(enabled);
      n++;
    } catch (_) {
    }
  }
  return n;
}
function toggleFly() {
  flyOn = !flyOn;
  if (flyOn) {
    applyGravity(0);
    const n = rigidbodiesGravity(false);
    flyTimer = setInterval(() => applyGravity(0), 1500);
    toast(`Fly ON (rb: ${n})`);
  } else {
    if (flyTimer) {
      clearInterval(flyTimer);
      flyTimer = null;
    }
    applyGravity(-9.81);
    rigidbodiesGravity(true);
    toast("Fly OFF");
  }
}
function writeFullDump(path) {
  let file;
  try {
    file = new File(path, "w");
  } catch (e) {
    toast("\u0434\u0430\u043C\u043F: \u043D\u0435 \u043E\u0442\u043A\u0440\u044B\u0442\u044C \u0444\u0430\u0439\u043B");
    return;
  }
  for (const asm of Il2Cpp.domain.assemblies) {
    let image;
    try {
      image = asm.image;
    } catch {
      continue;
    }
    file.write(`
// ===== ${image.name} =====
`);
    for (const klass of image.classes) {
      let fields, methods;
      try {
        fields = klass.fields;
        methods = klass.methods;
      } catch {
        continue;
      }
      file.write(`class ${klass.type.name} {
`);
      for (const f of fields) {
        try {
          file.write(`  ${f.isStatic ? "static " : ""}${f.type.name} ${f.name}; // 0x${f.offset.toString(16)}
`);
        } catch (_) {
        }
      }
      for (const m of methods) {
        try {
          file.write(`  ${m.isStatic ? "static " : ""}${m.returnType.name} ${m.name}(${m.parameterCount});
`);
        } catch (_) {
        }
      }
      file.write(`}
`);
    }
  }
  try {
    file.flush();
    file.close();
  } catch (_) {
  }
}
function dumpCs() {
  writeFullDump(DUMP_PATH);
  toast("\u0414\u0430\u043C\u043F: " + DUMP_PATH);
}
function searchField(q) {
  if (!q)
    return;
  const re = new RegExp(q, "i");
  let hits = 0;
  for (const image of gameImages()) {
    for (const klass of image.classes) {
      let fields;
      try {
        fields = klass.fields;
      } catch {
        continue;
      }
      for (const f of fields) {
        if (!re.test(f.name))
          continue;
        let tn = "?";
        try {
          tn = f.type.name;
        } catch {
        }
        console.log(`  ${f.isStatic ? "S" : "i"} ${image.name} | ${klass.type.name}.${f.name} : ${tn}`);
        if (++hits > 200) {
          console.log("  ...\u043E\u0431\u0440\u0435\u0437\u0430\u043D\u043E");
          toast("\u041D\u0430\u0439\u0434\u0435\u043D\u043E >200, \u0441\u043C. \u043A\u043E\u043D\u0441\u043E\u043B\u044C");
          return;
        }
      }
    }
  }
  toast(`\u041F\u043E\u0438\u0441\u043A "${q}": ${hits} (\u0441\u043C. \u043A\u043E\u043D\u0441\u043E\u043B\u044C)`);
}
function perform(fn) {
  try {
    Il2Cpp.perform(fn);
  } catch (e) {
    console.log("[il2cpp]", e);
  }
}
function buildUI() {
  const ActivityThread = Java.use("android.app.ActivityThread");
  const ActivityClientRecord = Java.use("android.app.ActivityThread$ActivityClientRecord");
  const FrameLayout = Java.use("android.widget.FrameLayout");
  const LinearLayout = Java.use("android.widget.LinearLayout");
  const TextView = Java.use("android.widget.TextView");
  const EditText = Java.use("android.widget.EditText");
  const Button = Java.use("android.widget.Button");
  const Toast = Java.use("android.widget.Toast");
  const Color = Java.use("android.graphics.Color");
  const Gravity = Java.use("android.view.Gravity");
  const View = Java.use("android.view.View");
  const ViewGroupLP = Java.use("android.view.ViewGroup$LayoutParams");
  const FrameLP = Java.use("android.widget.FrameLayout$LayoutParams");
  const LinearLP = Java.use("android.widget.LinearLayout$LayoutParams");
  const InputType = Java.use("android.text.InputType");
  const MotionEvent = Java.use("android.view.MotionEvent");
  const TypedValue = Java.use("android.util.TypedValue");
  const OnClickListener = Java.use("android.view.View$OnClickListener");
  const OnTouchListener = Java.use("android.view.View$OnTouchListener");
  function getTopActivity() {
    const at = ActivityThread.currentActivityThread();
    const map = at.mActivities.value;
    const vals = map.values().toArray();
    for (const r of vals) {
      const rec = Java.cast(r, ActivityClientRecord);
      if (!rec.paused.value)
        return rec.activity.value;
    }
    if (vals.length)
      return Java.cast(vals[0], ActivityClientRecord).activity.value;
    return null;
  }
  const activity = getTopActivity();
  if (!activity)
    throw new Error("activity not ready");
  const ctx = activity;
  const dp = (v) => Math.round(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP.value, v, ctx.getResources().getDisplayMetrics()));
  toast = (s) => {
    Java.scheduleOnMainThread(() => {
      try {
        Toast.makeText(ctx, "\u{1F383} " + s, Toast.LENGTH_SHORT.value).show();
      } catch (_) {
      }
    });
    console.log("[toast]", s);
  };
  const uid = () => Math.floor(Math.random() * 1e7);
  function onClick(fn) {
    const L = Java.registerClass({
      name: "noomi.C" + uid(),
      implements: [OnClickListener],
      methods: { onClick(_v) {
        perform(fn);
      } }
    });
    return L.$new();
  }
  const panel = LinearLayout.$new(ctx);
  panel.setOrientation(LinearLayout.VERTICAL.value);
  panel.setBackgroundColor(Color.parseColor("#E6241a1a"));
  panel.setPadding(dp(12), dp(12), dp(12), dp(12));
  panel.setVisibility(View.GONE.value);
  const title = TextView.$new(ctx);
  title.setText("NoomiCloneX  \u{1F383}");
  title.setTextColor(Color.parseColor("#FFB347"));
  title.setTextSize(18);
  panel.addView(title);
  function numRow(label, hint, onSet) {
    const row = LinearLayout.$new(ctx);
    row.setOrientation(LinearLayout.HORIZONTAL.value);
    const et = EditText.$new(ctx);
    et.setHint(hint);
    et.setTextColor(Color.WHITE.value);
    et.setInputType(InputType.TYPE_CLASS_NUMBER.value);
    et.setLayoutParams(LinearLP.$new(0, ViewGroupLP.WRAP_CONTENT.value, 1));
    const b = Button.$new(ctx);
    b.setText(label);
    b.setOnClickListener(onClick(() => {
      const t = et.getText().toString();
      onSet(t && t.length ? t : "0");
    }));
    row.addView(et);
    row.addView(b);
    panel.addView(row);
  }
  numRow("\u0414\u0435\u043D\u044C\u0433\u0438", "\u0441\u0443\u043C\u043C\u0430", setMoney);
  numRow("\u041F\u0435\u0440\u0435\u0445\u0432\u0430\u0442\u044B", "\u043A\u043E\u043B-\u0432\u043E", setIntercept);
  const flyBtn = Button.$new(ctx);
  flyBtn.setText("Fly: OFF");
  flyBtn.setOnClickListener(onClick(() => {
    toggleFly();
    Java.scheduleOnMainThread(() => flyBtn.setText(flyOn ? "Fly: ON" : "Fly: OFF"));
  }));
  panel.addView(flyBtn);
  const sRow = LinearLayout.$new(ctx);
  sRow.setOrientation(LinearLayout.HORIZONTAL.value);
  const sEt = EditText.$new(ctx);
  sEt.setHint("\u0438\u0441\u043A\u0430\u0442\u044C \u043F\u043E\u043B\u0435");
  sEt.setTextColor(Color.WHITE.value);
  sEt.setLayoutParams(LinearLP.$new(0, ViewGroupLP.WRAP_CONTENT.value, 1));
  const sBtn = Button.$new(ctx);
  sBtn.setText("\u041F\u043E\u0438\u0441\u043A");
  sBtn.setOnClickListener(onClick(() => searchField(sEt.getText().toString())));
  sRow.addView(sEt);
  sRow.addView(sBtn);
  panel.addView(sRow);
  const dBtn = Button.$new(ctx);
  dBtn.setText("Dump C#");
  dBtn.setOnClickListener(onClick(dumpCs));
  panel.addView(dBtn);
  const pumpkin = TextView.$new(ctx);
  pumpkin.setText("\u{1F383}");
  pumpkin.setTextSize(28);
  pumpkin.setBackgroundColor(Color.parseColor("#AA000000"));
  pumpkin.setPadding(dp(10), dp(6), dp(10), dp(6));
  const container = LinearLayout.$new(ctx);
  container.setOrientation(LinearLayout.VERTICAL.value);
  container.addView(pumpkin);
  container.addView(panel);
  const ts = { x0: 0, y0: 0, tx: 0, ty: 0, moved: false };
  const touchL = Java.registerClass({
    name: "noomi.T" + uid(),
    implements: [OnTouchListener],
    methods: {
      onTouch(_v, ev) {
        const a = ev.getActionMasked();
        if (a === MotionEvent.ACTION_DOWN.value) {
          ts.x0 = ev.getRawX();
          ts.y0 = ev.getRawY();
          ts.tx = container.getTranslationX();
          ts.ty = container.getTranslationY();
          ts.moved = false;
          return true;
        } else if (a === MotionEvent.ACTION_MOVE.value) {
          const dx = ev.getRawX() - ts.x0, dy = ev.getRawY() - ts.y0;
          if (Math.abs(dx) > 10 || Math.abs(dy) > 10)
            ts.moved = true;
          container.setTranslationX(ts.tx + dx);
          container.setTranslationY(ts.ty + dy);
          return true;
        } else if (a === MotionEvent.ACTION_UP.value) {
          if (!ts.moved) {
            const vis = panel.getVisibility();
            panel.setVisibility(vis === View.GONE.value ? View.VISIBLE.value : View.GONE.value);
          }
          return true;
        }
        return false;
      }
    }
  });
  pumpkin.setOnTouchListener(touchL.$new());
  const decor = activity.getWindow().getDecorView();
  const content = Java.cast(decor, FrameLayout);
  const clp = FrameLP.$new(ViewGroupLP.WRAP_CONTENT.value, ViewGroupLP.WRAP_CONTENT.value);
  clp.gravity.value = Gravity.TOP.value | Gravity.START.value;
  clp.leftMargin.value = dp(8);
  clp.topMargin.value = dp(80);
  content.addView(container, clp);
  console.log("[+] \u041C\u0435\u043D\u044E-\u0442\u044B\u043A\u0432\u0430 \u043F\u0440\u0438\u043A\u0440\u0435\u043F\u043B\u0435\u043D\u043E.");
  toast("\u043C\u0435\u043D\u044E \u0433\u043E\u0442\u043E\u0432\u043E");
}
var built = false;
function tryBuild(tries) {
  if (built)
    return;
  Java.perform(() => {
    Java.scheduleOnMainThread(() => {
      if (built)
        return;
      try {
        buildUI();
        built = true;
      } catch (e) {
        if (tries % 4 === 0)
          console.log("[..] \u0436\u0434\u0443 \u0430\u043A\u0442\u0438\u0432\u0438\u0442\u0438 \u0438\u0433\u0440\u044B...");
      }
    });
  });
  if (!built && tries < 120)
    setTimeout(() => tryBuild(tries + 1), 1500);
}
Il2Cpp.perform(() => {
  console.log("[*] il2cpp \u0433\u043E\u0442\u043E\u0432. Unity", Il2Cpp.unityVersion);
  tryBuild(0);
});

✄
{
  "version": 3,
  "sources": ["node_modules/frida-il2cpp-bridge/dist/application.ts", "node_modules/frida-il2cpp-bridge/dist/boxed.ts", "node_modules/frida-il2cpp-bridge/dist/config.ts", "node_modules/frida-il2cpp-bridge/dist/exception-listener.ts", "node_modules/frida-il2cpp-bridge/dist/exports.ts", "node_modules/frida-il2cpp-bridge/dist/filters.ts", "node_modules/frida-il2cpp-bridge/dist/gc.ts", "node_modules/frida-il2cpp-bridge/dist/utils/android.ts", "node_modules/frida-il2cpp-bridge/dist/utils/console.ts", "node_modules/frida-il2cpp-bridge/dist/utils/decorate.ts", "node_modules/frida-il2cpp-bridge/dist/utils/getter.ts", "node_modules/frida-il2cpp-bridge/dist/utils/lazy.ts", "node_modules/frida-il2cpp-bridge/dist/utils/native-struct.ts", "node_modules/frida-il2cpp-bridge/dist/utils/object.ts", "node_modules/frida-il2cpp-bridge/dist/utils/offset-of.ts", "node_modules/frida-il2cpp-bridge/dist/utils/read-native-iterator.ts", "node_modules/frida-il2cpp-bridge/dist/utils/read-native-list.ts", "node_modules/frida-il2cpp-bridge/dist/utils/recycle.ts", "node_modules/frida-il2cpp-bridge/dist/utils/unity-version.ts", "node_modules/frida-il2cpp-bridge/dist/memory.ts", "node_modules/frida-il2cpp-bridge/dist/module.ts", "node_modules/frida-il2cpp-bridge/dist/nullable.ts", "node_modules/frida-il2cpp-bridge/dist/perform.ts", "node_modules/frida-il2cpp-bridge/dist/tracer.ts", "node_modules/frida-il2cpp-bridge/dist/structs/array.ts", "node_modules/frida-il2cpp-bridge/dist/structs/assembly.ts", "node_modules/frida-il2cpp-bridge/dist/structs/class.ts", "node_modules/frida-il2cpp-bridge/dist/structs/delegate.ts", "node_modules/frida-il2cpp-bridge/dist/structs/domain.ts", "node_modules/frida-il2cpp-bridge/dist/structs/field.ts", "node_modules/frida-il2cpp-bridge/dist/structs/gc-handle.ts", "node_modules/frida-il2cpp-bridge/dist/structs/image.ts", "node_modules/frida-il2cpp-bridge/dist/structs/memory-snapshot.ts", "node_modules/frida-il2cpp-bridge/dist/structs/method.ts", "node_modules/frida-il2cpp-bridge/dist/structs/object.ts", "node_modules/frida-il2cpp-bridge/dist/structs/parameter.ts", "node_modules/frida-il2cpp-bridge/dist/structs/pointer.ts", "node_modules/frida-il2cpp-bridge/dist/structs/reference.ts", "node_modules/frida-il2cpp-bridge/dist/structs/string.ts", "node_modules/frida-il2cpp-bridge/dist/structs/thread.ts", "node_modules/frida-il2cpp-bridge/dist/structs/type.ts", "node_modules/frida-il2cpp-bridge/dist/structs/value-type.ts", "node_modules/frida-il2cpp-bridge/dist/index.ts", "src/menu.ts"],
  "mappings": "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",
  "names": ["Il2Cpp", "Android", "getter", "obj", "UnityVersion", "a", "b", "Il2Cpp", "_", "module", "perform", "parameters", "method", "Array", "array", "delegate", "memorySnapshot", "i", "Object", "field", "value"]
}
