#! /usr/bin/env bash
set -eu
shopt -s nullglob

imported=""
# Generate all.djinni
for var in "$@"
do
  imported="$imported@import \"$var\"\n"
done

# output dir should already exist
printf "$imported" > output/all.djinni

# Locate the script file.  Cross symlinks if necessary.
loc="$0"
while [ -h "$loc" ]; do
    ls=`ls -ld "$loc"`
    link=`expr "$ls" : '.*-> \(.*\)$'`
    if expr "$link" : '/.*' > /dev/null; then
        loc="$link"  # Absolute link
    else
        loc="`dirname "$loc"`/$link"  # Relative link
    fi
done
base_dir=$(cd "`dirname "$loc"`" && pwd)

temp_out="$base_dir/djinni-output-temp"

in="$base_dir/example.djinni"

cpp_out="$base_dir/generated-src/cpp"
jni_out="$base_dir/generated-src/jni"
objc_out="$base_dir/generated-src/objc"
java_out="$base_dir/modules/superglue/src/main/java/com/loki/superglue/djinni/jni"

java_package="com.loki.superglue.djinni.jni"

gen_stamp="$temp_out/gen.stamp"

if [ $# -eq 0 ]; then
    # Normal build.
    true
elif [ $# -eq 1 ]; then
    command="$1"; shift
    if [ "$command" != "clean" ]; then
        echo "Unexpected argument: \"$command\"." 1>&2
        exit 1
    fi
    for dir in "$temp_out" "$cpp_out" "$jni_out" "$java_out"; do
        if [ -e "$dir" ]; then
            echo "Deleting \"$dir\"..."
            rm -r "$dir"
        fi
    done
    exit
fi

# Build djinni
"$base_dir/djinni/src/build"

[ ! -e "$temp_out" ] || rm -r "$temp_out"
"$base_dir/djinni/src/run-assume-built" \
    --java-out "$temp_out/java" \
    --java-package $java_package \
    --java-nullable-annotation "javax.annotation.CheckForNull" \
    --java-nonnull-annotation "javax.annotation.Nonnull" \
    --ident-java-field mFooBar \
    \
    --cpp-out "$temp_out/cpp" \
    --cpp-namespace "gen" \
    --ident-cpp-enum-type foo_bar \
    --cpp-optional-template "util::Optional"
    --cpp-optional-header "<kitty/util/optional.h>"
    \
    --jni-out "$temp_out/jni" \
    --ident-jni-class Native \
    --ident-jni-file Native \
    \
      --objcpp-namespace djinni_generated \
      --objc-out "$temp_out/objc" \
      --objcpp-out "$temp_out/objc" \
    --objc-type-prefix "u" \
    \
    --idl "$in"
# Copy changes from "$temp_output" to final dir.

mirror() {
    local prefix="$1" ; shift
    local src="$1" ; shift
    local dest="$1" ; shift
    mkdir -p "$dest"
    rsync -a --delete --checksum --itemize-changes "$src"/ "$dest" | grep -v '^\.' | sed "s/^/[$prefix]/"
}

echo "Copying generated code to final directories..."
mirror "cpp" "$temp_out/cpp" "$cpp_out"
mirror "java" "$temp_out/java" "$java_out"
mirror "jni" "$temp_out/jni" "$jni_out"
mirror "objc" "$temp_out/objc" "$objc_out"

date > "$gen_stamp"

echo "djinni completed."
