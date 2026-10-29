# بناء محرّك FET (fet-cl) في حاوية — لا يحتاج Qt على جهازك.
#
#   docker build -f docker/engine.Dockerfile -t open-timetable-engine .
#   docker run --rm -v "$PWD/engine/samples:/in:ro" -v "$PWD/fet-runs:/out" \
#     open-timetable-engine fet-cl --inputfile=/in/small-test.fet --outputdir=/out --timelimitseconds=60
#
# ملاحظة: نستخدم qmake + Qt 5.15 (متوفرة في مستودعات Ubuntu 22.04) لأن مسار CMake في FET 7.12
# يشترط Qt 6.10+؛ كود FET يدعم Qt 5.15 أيضًا (src/src-cl.pro يفرض 5.15 كحد أدنى).

# ---------- مرحلة البناء ----------
FROM ubuntu:22.04 AS build

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      build-essential qtbase5-dev ca-certificates tar xz-utils \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY vendor/fet/fet-7.12.0.tar.xz vendor/fet/fet-7.12.0.tar.xz.asc vendor/fet/SHA256SUMS ./vendor/fet/
COPY scripts ./scripts
RUN sha256sum -c vendor/fet/SHA256SUMS \
 && mkdir -p .fet-src \
 && tar -xJf vendor/fet/fet-7.12.0.tar.xz -C .fet-src \
 && mkdir -p build/cl-build \
 && cd build/cl-build \
 && qmake /src/.fet-src/fet-7.12.0/src/src-cl.pro CONFIG+=release \
 && make -j"$(nproc)" \
 && ls -l /src/build/fet-cl

# ---------- مرحلة التشغيل ----------
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
 && apt-get install -y --no-install-recommends libqt5core5a ca-certificates \
 && rm -rf /var/lib/apt/lists/*

COPY --from=build /src/build/fet-cl /usr/local/bin/fet-cl

# مستخدم غير جذر (المحرّك يكتب مخرجات في /out)
RUN useradd --create-home --uid 10001 fet && mkdir -p /out && chown fet /out
USER fet
WORKDIR /out

ENTRYPOINT ["fet-cl"]
CMD ["--version"]
