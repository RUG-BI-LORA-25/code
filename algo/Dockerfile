# Build stage
FROM python:3.11-slim AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    python3-dev \
    build-essential

WORKDIR /app

COPY . .

RUN pip install -r requirements.txt

CMD ["make","run"]
