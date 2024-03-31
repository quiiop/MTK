
/**
  * Used for Hardware Encoding/Decoding LC3 codec parameters.
  */
struct bt_Lc3Parameters {
    /* PCM is Input for encoder, Output for decoder */
    uint8_t pcmBitDepth;
    /* codec-specific parameters */
    uint32_t samplingFrequency;
    uint8_t frameDuration;
    /* length in octets of a codec frame */
    uint32_t octetsPerFrame;
    /* Number of blocks of codec frames per single SDU (Service Data Unit) */
    uint8_t blocksPerSdu;
};

/**
  * Used to configure a LC3 Hardware Encoding session.
  */

struct bt_LeAudioCodecConfiguration {
    uint32_t codecType;
    /* This is also bitfield, specifying how the channels are ordered in the outgoing media packet.
    * Bit meaning is defined in Bluetooth Assigned Numbers. */
    uint32_t audioChannelAllocation;
    uint32_t encodedAudioBitrate;
    bool bfi_ext; //Bad Frame Indication Flag
    uint8_t plc_method; //Packet Loss Concealment Algorithm
    uint8_t le_audio_type; // 0: UMS ,1: CG , 2: BMS
    uint32_t presentation_delay;//us
    struct bt_Lc3Parameters lc3Config;
};

struct bt_ConnParam {
    uint32_t conn_handle_R;
    uint32_t conn_handle_L;
    uint8_t bn;  // Burst Number
};


