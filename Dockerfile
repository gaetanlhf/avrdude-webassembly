FROM emscripten/emsdk:3.1.74

RUN apt-get update && apt-get install -y --no-install-recommends \
    flex \
    bison \
    && rm -rf /var/lib/apt/lists/*

RUN npm install -g yarn

WORKDIR /src

COPY package.json yarn.lock ./
RUN yarn install --frozen-lockfile

COPY . .

RUN cmake -B build -S . \
    -DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
    && cmake --build build --target avrdude \
    && cp build/libserial/avrdude-worker.js build/src/avrdude.js build/src/avrdude.wasm build/src/avrdude.conf .