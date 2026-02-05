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
    cmake .. && \
    make -j$(nproc) && \
    make DESTDIR=/install install

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

# Copy everything from the install directory
COPY --from=builder /install/usr/local /usr/local

# Create symlinks and update library cache
RUN ldconfig

ENV PYTHONPATH="/usr/local/lib/python3.11/dist-packages:${PYTHONPATH}"
ENV LD_LIBRARY_PATH="/usr/local/lib:${LD_LIBRARY_PATH}"

WORKDIR /app

COPY . .

RUN chmod +x run.py

CMD ["python3", "run.py"]
