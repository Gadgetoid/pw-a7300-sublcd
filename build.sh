#!/bin/sh
set -eu

JORNADA_TOOLS_REPOSITORY=https://github.com/Gadgetoid/jornada-72x-tools.git
JORNADA_TOOLS_COMMIT=71f74221832c0551f8df257491f5b94a771d4057
LLVM_URL=https://github.com/Gadgetoid/velo-llvm-builds/releases/download/v3/velo-llvm-v3-linux-x86_64.tar.xz
LLVM_SHA256=c3f0b67e22105ef68bbb71d0a316b268ef3e44e5b83aab93b0f9a071d2706b77
BUILDER_IMAGE=debian@sha256:62bd423aeee912639ca8ee681c2b762cb3e25137e0a92b20be455898a9f034ad
DEBIAN_SNAPSHOT=20261023T000000Z

repository=$(cd "$(dirname "$0")" && pwd)
output_directory=${1:-$repository/out}
mkdir -p "$output_directory"
output_directory=$(cd "$output_directory" && pwd)

if [ "${IN_BUILDER:-}" != 1 ]; then
    exec docker run --rm --platform linux/amd64 \
        -e IN_BUILDER=1 -e HOST_UID="$(id -u)" -e HOST_GID="$(id -g)" \
        -v "$repository:/source:ro" -v "$output_directory:/out" \
        "$BUILDER_IMAGE" sh /source/build.sh /out
fi

export DEBIAN_FRONTEND=noninteractive
rm -f /etc/apt/sources.list.d/*
cat > /etc/apt/sources.list <<SOURCES
deb [check-valid-until=no] http://snapshot.debian.org/archive/debian/$DEBIAN_SNAPSHOT trixie main
deb [check-valid-until=no] http://snapshot.debian.org/archive/debian/$DEBIAN_SNAPSHOT trixie-updates main
deb [check-valid-until=no] http://snapshot.debian.org/archive/debian-security/$DEBIAN_SNAPSHOT trixie-security main
SOURCES
apt-get -o Acquire::Retries=3 update -qq
apt-get install -y -qq --no-install-recommends \
    ca-certificates curl git python3 xz-utils cmake ninja-build nodejs > /dev/null

work=/work
mkdir -p "$work/jornada-72x-tools" "$work/llvm"
git -C "$work/jornada-72x-tools" init -q
git -C "$work/jornada-72x-tools" fetch -q --depth 1 "$JORNADA_TOOLS_REPOSITORY" "$JORNADA_TOOLS_COMMIT"
git -C "$work/jornada-72x-tools" checkout -q FETCH_HEAD
curl -sSfL -o "$work/llvm.tar.xz" "$LLVM_URL"
echo "$LLVM_SHA256  $work/llvm.tar.xz" | sha256sum -c --quiet
tar -xJf "$work/llvm.tar.xz" -C "$work/llvm" --strip-components=1

cmake -S /source -B "$work/build" -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DCMAKE_TOOLCHAIN_FILE="$work/jornada-72x-tools/cmake/jornada-ce.cmake" \
    -DJORNADA_TOOLCHAIN="$work/jornada-72x-tools" -DJORNADA_LLVM_ROOT="$work/llvm" > /dev/null
cmake --build "$work/build"
rm -rf /out/card
cp -R "$work/build/card" /out/card

{
    echo "jornada-72x-tools $JORNADA_TOOLS_COMMIT"
    echo "llvm $LLVM_URL $LLVM_SHA256"
    echo "image $BUILDER_IMAGE"
    echo "debian snapshot $DEBIAN_SNAPSHOT"
    cd /out && find card -type f -name '*.exe' | sort | xargs -d '\n' sha256sum
} > /out/build-info.txt
chown -R "$HOST_UID:$HOST_GID" /out
cat /out/build-info.txt
