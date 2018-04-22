# What's jnativetracer?

jnativetracer is JVMTI agent to check callstack(s) to native (JNI) call.  
You can get all callstacks which include native call.

# How to use

## Build

```
$ make JAVA_HOME=/path/to/jdk
```

## Check callstacks

Pass `-agentpath:/path/to/libjnativetracer.so` to `java`.

### Options

* `dumpstack=true`
    * Dump stack traces in each native calls.
* `trigger=<class>::<field>`
    * Set trigger field. Field should be `boolean` type. If this field set to `true`, jnativetracer starts tracing all native calls, and this field set to `false`, jnativetracer stops tracing.

# Example

```
$ cd example
$ javac SystemGC.java
$ java -agentpath:/path/to/libjnativetracer.so=dumpstack=true,trigger=SystemGC::isTrace SystemGC
```

# TODO

`FieldModification` JVMTI event might be called in multi-threaded, but jnativetracer is not considered yet. `FieldModification` and `MethodEntry` should check `jthread` value or TID.

# License

The GNU Lesser General Public License, version 3.0
