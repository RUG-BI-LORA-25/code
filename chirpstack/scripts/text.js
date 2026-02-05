function decodeUplink(input) {
    return {
        data: {
            message: String.fromCharCode.apply(null, input.bytes)
        }
    };
}