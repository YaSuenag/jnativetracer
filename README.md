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
* `global=false`
    * Show native call info in `trigger` thread only.
    * This option affects when `trigger` is set.

# Example

```
$ cd example
$ javac SystemGC.java
$ java -agentpath:/path/to/libjnativetracer.so=global=false,dumpstack=true,trigger=SystemGC::isTrace SystemGC
```

# License

The GNU Lesser General Public License, version 3.0
