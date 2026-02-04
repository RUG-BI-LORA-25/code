# Build stage
FROM debian:bookworm-slim AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    ca-certificates \
    pkg-config \
    python3-dev \
    gnuradio-dev \
    libboost-all-dev \
    libspdlog-dev \
    libfmt-dev \
    pybind11-dev \
    libvolk2-dev \
    && rm -rf /var/lib/apt/lists/*

# Build gr-lora_sdr
WORKDIR /tmp
RUN git clone --depth 1 https://github.com/tapparelj/gr-lora_sdr.git && \
    cd gr-lora_sdr && \
    mkdir build && cd build && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr .. && \
    make -j$(nproc) && \
    make install

# Runtime stage
FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    python3 \
    python3-numpy \
    gnuradio \
    gr-osmosdr \
    hackrf \
    libspdlog1.10 \
    libfmt9 \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

COPY --from=builder /usr/lib/python3/dist-packages/gnuradio/lora_sdr /usr/lib/python3/dist-packages/gnuradio/lora_sdr
COPY --from=builder /usr/lib/*/libgnuradio-lora_sdr* /usr/lib/

RUN ldconfig

WORKDIR /app

COPY config.py .
COPY forwarder.py .
COPY receiver.py .
COPY transmitter.py .
COPY run.py .
COPY cartof.iqhackrf .

RUN chmod +x run.py

CMD ["python3", "run.py"]
