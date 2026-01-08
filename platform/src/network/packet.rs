//! Packet type definitions for game protocol
//!
//! Provides structures compatible with the original Red Alert network protocol.
//! The original game used:
//! - CommHeaderType: 7-byte packet header
//! - Various command types for game state sync
//!
//! We adapt this to work with enet's reliable/unreliable channels.

/// Packet delivery mode
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum DeliveryMode {
    /// Reliable, ordered delivery (enet channel 0)
    /// Used for game commands, chat, important state
    #[default]
    Reliable = 0,

    /// Unreliable delivery (enet channel 1)
    /// Used for frequent updates where latest data matters more than completeness
    Unreliable = 1,

    /// Unreliable, unsequenced (enet flag)
    /// Packets may arrive out of order
    Unsequenced = 2,
}

impl DeliveryMode {
    /// Convert to enet channel number
    pub fn to_channel(self) -> u8 {
        match self {
            DeliveryMode::Reliable => 0,
            DeliveryMode::Unreliable | DeliveryMode::Unsequenced => 1,
        }
    }

    /// Check if this mode requires reliable delivery
    pub fn is_reliable(self) -> bool {
        matches!(self, DeliveryMode::Reliable)
    }
}

/// Magic number for packet validation
/// Original: 0xDABD for serial, various for network
pub const PACKET_MAGIC: u16 = 0xC0DE;

/// Game packet header
///
/// Based on original CommHeaderType but adapted for cross-platform use.
/// Uses explicit byte sizes rather than platform-dependent types.
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct GamePacketHeader {
    /// Magic number for validation (2 bytes)
    pub magic: u16,
    /// Packet type code (1 byte)
    pub code: u8,
    /// Sequence/packet ID (4 bytes)
    pub sequence: u32,
    /// Source player ID (1 byte)
    pub player_id: u8,
    /// Flags (1 byte) - reserved for future use
    pub flags: u8,
}

impl GamePacketHeader {
    /// Header size in bytes
    pub const SIZE: usize = 10;

    /// Create a new packet header
    pub fn new(code: GamePacketCode, sequence: u32, player_id: u8) -> Self {
        Self {
            magic: PACKET_MAGIC,
            code: code as u8,
            sequence,
            player_id,
            flags: 0,
        }
    }

    /// Check if header magic is valid
    pub fn is_valid(&self) -> bool {
        self.magic == PACKET_MAGIC
    }

    /// Get the packet code as enum
    pub fn packet_code(&self) -> GamePacketCode {
        GamePacketCode::from(self.code)
    }

    /// Serialize header to bytes
    pub fn to_bytes(&self) -> [u8; Self::SIZE] {
        let mut bytes = [0u8; Self::SIZE];
        bytes[0..2].copy_from_slice(&self.magic.to_le_bytes());
        bytes[2] = self.code;
        bytes[3..7].copy_from_slice(&self.sequence.to_le_bytes());
        bytes[7] = self.player_id;
        bytes[8] = self.flags;
        bytes[9] = 0; // Padding for alignment
        bytes
    }

    /// Deserialize header from bytes
    pub fn from_bytes(bytes: &[u8]) -> Option<Self> {
        if bytes.len() < Self::SIZE {
            return None;
        }

        let magic = u16::from_le_bytes([bytes[0], bytes[1]]);
        let code = bytes[2];
        let sequence = u32::from_le_bytes([bytes[3], bytes[4], bytes[5], bytes[6]]);
        let player_id = bytes[7];
        let flags = bytes[8];

        Some(Self {
            magic,
            code,
            sequence,
            player_id,
            flags,
        })
    }
}

impl Default for GamePacketHeader {
    fn default() -> Self {
        Self {
            magic: PACKET_MAGIC,
            code: 0,
            sequence: 0,
            player_id: 0,
            flags: 0,
        }
    }
}

/// Game packet codes
///
/// Based on original game's NetCommandType and connection codes.
#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum GamePacketCode {
    // =========================
    // Connection Management
    // =========================
    /// Data packet requiring acknowledgment
    #[default]
    DataAck = 0,

    /// Data packet not requiring ack
    DataNoAck = 1,

    /// Acknowledgment packet
    Ack = 2,

    // =========================
    // Game Discovery
    // =========================
    /// Request game information (broadcast)
    RequestGameInfo = 10,

    /// Game information response
    GameInfo = 11,

    /// Request to join a game
    JoinRequest = 12,

    /// Join accepted
    JoinAccepted = 13,

    /// Join rejected
    JoinRejected = 14,

    // =========================
    // Game Session
    // =========================
    /// Player info update
    PlayerInfo = 20,

    /// Game options/scenario info
    ScenarioInfo = 21,

    /// Chat message
    ChatMessage = 22,

    /// Player ready status
    ReadyStatus = 23,

    /// Game start signal
    GameStart = 24,

    // =========================
    // Gameplay
    // =========================
    /// Game commands/actions
    GameCommands = 30,

    /// Game sync packet
    GameSync = 31,

    /// Frame/timing sync
    FrameSync = 32,

    /// Game over / results
    GameOver = 33,

    // =========================
    // Keep-alive
    // =========================
    /// Ping request
    Ping = 100,

    /// Pong response
    Pong = 101,

    // =========================
    // System
    // =========================
    /// Disconnect notification
    Disconnect = 200,

    /// Error notification
    Error = 201,

    /// Unknown/invalid
    Unknown = 255,
}

impl From<u8> for GamePacketCode {
    fn from(value: u8) -> Self {
        match value {
            0 => GamePacketCode::DataAck,
            1 => GamePacketCode::DataNoAck,
            2 => GamePacketCode::Ack,
            10 => GamePacketCode::RequestGameInfo,
            11 => GamePacketCode::GameInfo,
            12 => GamePacketCode::JoinRequest,
            13 => GamePacketCode::JoinAccepted,
            14 => GamePacketCode::JoinRejected,
            20 => GamePacketCode::PlayerInfo,
            21 => GamePacketCode::ScenarioInfo,
            22 => GamePacketCode::ChatMessage,
            23 => GamePacketCode::ReadyStatus,
            24 => GamePacketCode::GameStart,
            30 => GamePacketCode::GameCommands,
            31 => GamePacketCode::GameSync,
            32 => GamePacketCode::FrameSync,
            33 => GamePacketCode::GameOver,
            100 => GamePacketCode::Ping,
            101 => GamePacketCode::Pong,
            200 => GamePacketCode::Disconnect,
            201 => GamePacketCode::Error,
            _ => GamePacketCode::Unknown,
        }
    }
}

/// A complete game packet (header + payload)
#[derive(Debug, Clone)]
pub struct GamePacket {
    /// Packet header
    pub header: GamePacketHeader,
    /// Packet payload data
    pub payload: Vec<u8>,
}

impl GamePacket {
    /// Create a new packet with the given code and payload
    pub fn new(code: GamePacketCode, sequence: u32, player_id: u8, payload: Vec<u8>) -> Self {
        Self {
            header: GamePacketHeader::new(code, sequence, player_id),
            payload,
        }
    }

    /// Create an empty packet with just a code
    pub fn empty(code: GamePacketCode, sequence: u32, player_id: u8) -> Self {
        Self::new(code, sequence, player_id, Vec::new())
    }

    /// Total size when serialized
    pub fn total_size(&self) -> usize {
        GamePacketHeader::SIZE + self.payload.len()
    }

    /// Serialize packet to bytes
    pub fn serialize(&self) -> Vec<u8> {
        let mut buffer = Vec::with_capacity(self.total_size());
        buffer.extend_from_slice(&self.header.to_bytes());
        buffer.extend_from_slice(&self.payload);
        buffer
    }

    /// Deserialize packet from bytes
    pub fn deserialize(data: &[u8]) -> Option<Self> {
        let header = GamePacketHeader::from_bytes(data)?;

        if !header.is_valid() {
            return None;
        }

        let payload = if data.len() > GamePacketHeader::SIZE {
            data[GamePacketHeader::SIZE..].to_vec()
        } else {
            Vec::new()
        };

        Some(Self { header, payload })
    }

    /// Get a reference to the payload
    pub fn payload(&self) -> &[u8] {
        &self.payload
    }

    /// Get the packet code
    pub fn code(&self) -> GamePacketCode {
        self.header.packet_code()
    }
}

// =============================================================================
// Utility Functions
// =============================================================================

/// Serialize a game packet to a byte vector
pub fn serialize_packet(header: &GamePacketHeader, payload: &[u8]) -> Vec<u8> {
    let mut buffer = Vec::with_capacity(GamePacketHeader::SIZE + payload.len());
    buffer.extend_from_slice(&header.to_bytes());
    buffer.extend_from_slice(payload);
    buffer
}

/// Deserialize a game packet from bytes
///
/// Returns (header, payload_slice) if successful
pub fn deserialize_packet(data: &[u8]) -> Option<(GamePacketHeader, &[u8])> {
    if data.len() < GamePacketHeader::SIZE {
        return None;
    }

    let header = GamePacketHeader::from_bytes(data)?;

    if !header.is_valid() {
        return None;
    }

    let payload = &data[GamePacketHeader::SIZE..];
    Some((header, payload))
}

/// Create a ping packet
pub fn create_ping_packet(sequence: u32, player_id: u8) -> GamePacket {
    GamePacket::empty(GamePacketCode::Ping, sequence, player_id)
}

/// Create a pong packet (response to ping)
pub fn create_pong_packet(ping_sequence: u32, player_id: u8) -> GamePacket {
    // Include the original ping sequence in payload
    let payload = ping_sequence.to_le_bytes().to_vec();
    GamePacket::new(GamePacketCode::Pong, ping_sequence, player_id, payload)
}

/// Create a chat message packet
pub fn create_chat_packet(sequence: u32, player_id: u8, message: &str) -> GamePacket {
    GamePacket::new(
        GamePacketCode::ChatMessage,
        sequence,
        player_id,
        message.as_bytes().to_vec(),
    )
}

// =============================================================================
// FFI Exports
// =============================================================================

/// Create a game packet header
///
/// Returns 0 on success, writes header to output buffer
#[no_mangle]
pub unsafe extern "C" fn Platform_Packet_CreateHeader(
    code: u8,
    sequence: u32,
    player_id: u8,
    out_header: *mut u8,
    out_size: *mut i32,
) -> i32 {
    if out_header.is_null() {
        return -1;
    }

    let header = GamePacketHeader::new(GamePacketCode::from(code), sequence, player_id);
    let bytes = header.to_bytes();

    std::ptr::copy_nonoverlapping(bytes.as_ptr(), out_header, GamePacketHeader::SIZE);

    if !out_size.is_null() {
        *out_size = GamePacketHeader::SIZE as i32;
    }

    0
}

/// Get the size of a packet header
#[no_mangle]
pub extern "C" fn Platform_Packet_HeaderSize() -> i32 {
    GamePacketHeader::SIZE as i32
}

/// Validate a packet header
///
/// Returns 1 if valid, 0 if invalid
#[no_mangle]
pub unsafe extern "C" fn Platform_Packet_ValidateHeader(data: *const u8, size: i32) -> i32 {
    if data.is_null() || size < GamePacketHeader::SIZE as i32 {
        return 0;
    }

    let slice = std::slice::from_raw_parts(data, size as usize);
    match GamePacketHeader::from_bytes(slice) {
        Some(header) if header.is_valid() => 1,
        _ => 0,
    }
}

/// Extract header fields from raw packet data
///
/// Returns 0 on success, -1 on error
#[no_mangle]
pub unsafe extern "C" fn Platform_Packet_ParseHeader(
    data: *const u8,
    size: i32,
    out_code: *mut u8,
    out_sequence: *mut u32,
    out_player_id: *mut u8,
) -> i32 {
    if data.is_null() || size < GamePacketHeader::SIZE as i32 {
        return -1;
    }

    let slice = std::slice::from_raw_parts(data, size as usize);
    let header = match GamePacketHeader::from_bytes(slice) {
        Some(h) if h.is_valid() => h,
        _ => return -1,
    };

    if !out_code.is_null() {
        *out_code = header.code;
    }
    if !out_sequence.is_null() {
        *out_sequence = header.sequence;
    }
    if !out_player_id.is_null() {
        *out_player_id = header.player_id;
    }

    0
}

/// Get pointer to payload data within a packet
///
/// Returns pointer to payload start, or null on error
/// Sets out_payload_size to payload size
#[no_mangle]
pub unsafe extern "C" fn Platform_Packet_GetPayload(
    data: *const u8,
    size: i32,
    out_payload_size: *mut i32,
) -> *const u8 {
    if data.is_null() || size <= GamePacketHeader::SIZE as i32 {
        if !out_payload_size.is_null() {
            *out_payload_size = 0;
        }
        return std::ptr::null();
    }

    let payload_size = size - GamePacketHeader::SIZE as i32;
    if !out_payload_size.is_null() {
        *out_payload_size = payload_size;
    }

    data.add(GamePacketHeader::SIZE)
}

/// Build a complete packet from header and payload
///
/// Returns bytes written, or -1 on error
#[no_mangle]
pub unsafe extern "C" fn Platform_Packet_Build(
    code: u8,
    sequence: u32,
    player_id: u8,
    payload: *const u8,
    payload_size: i32,
    out_buffer: *mut u8,
    buffer_size: i32,
) -> i32 {
    let total_size = GamePacketHeader::SIZE + payload_size.max(0) as usize;

    if out_buffer.is_null() || buffer_size < total_size as i32 {
        return -1;
    }

    // Write header
    let header = GamePacketHeader::new(GamePacketCode::from(code), sequence, player_id);
    let header_bytes = header.to_bytes();
    std::ptr::copy_nonoverlapping(header_bytes.as_ptr(), out_buffer, GamePacketHeader::SIZE);

    // Write payload if present
    if !payload.is_null() && payload_size > 0 {
        std::ptr::copy_nonoverlapping(
            payload,
            out_buffer.add(GamePacketHeader::SIZE),
            payload_size as usize,
        );
    }

    total_size as i32
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_header_serialization() {
        let header = GamePacketHeader::new(GamePacketCode::ChatMessage, 12345, 3);

        let bytes = header.to_bytes();
        assert_eq!(bytes.len(), GamePacketHeader::SIZE);

        let parsed = GamePacketHeader::from_bytes(&bytes).unwrap();
        assert_eq!(parsed.magic, PACKET_MAGIC);
        assert_eq!(parsed.code, GamePacketCode::ChatMessage as u8);
        assert_eq!(parsed.sequence, 12345);
        assert_eq!(parsed.player_id, 3);
        assert!(parsed.is_valid());
    }

    #[test]
    fn test_header_invalid_magic() {
        let mut bytes = GamePacketHeader::default().to_bytes();
        bytes[0] = 0xFF; // Corrupt magic number
        bytes[1] = 0xFF;

        let parsed = GamePacketHeader::from_bytes(&bytes).unwrap();
        assert!(!parsed.is_valid());
    }

    #[test]
    fn test_packet_serialization() {
        let payload = b"Hello, World!";
        let packet = GamePacket::new(GamePacketCode::ChatMessage, 42, 1, payload.to_vec());

        let serialized = packet.serialize();
        assert_eq!(serialized.len(), GamePacketHeader::SIZE + payload.len());

        let deserialized = GamePacket::deserialize(&serialized).unwrap();
        assert_eq!(deserialized.header.code, GamePacketCode::ChatMessage as u8);
        assert_eq!(deserialized.header.sequence, 42);
        assert_eq!(deserialized.header.player_id, 1);
        assert_eq!(deserialized.payload, payload);
    }

    #[test]
    fn test_empty_packet() {
        let packet = GamePacket::empty(GamePacketCode::Ping, 1, 0);

        let serialized = packet.serialize();
        assert_eq!(serialized.len(), GamePacketHeader::SIZE);

        let deserialized = GamePacket::deserialize(&serialized).unwrap();
        assert!(deserialized.payload.is_empty());
    }

    #[test]
    fn test_serialize_deserialize_functions() {
        let header = GamePacketHeader::new(GamePacketCode::GameCommands, 999, 2);
        let payload = [1u8, 2, 3, 4, 5];

        let serialized = serialize_packet(&header, &payload);
        let (parsed_header, parsed_payload) = deserialize_packet(&serialized).unwrap();

        assert_eq!(parsed_header.sequence, 999);
        assert_eq!(parsed_header.player_id, 2);
        assert_eq!(parsed_payload, &payload);
    }

    #[test]
    fn test_packet_code_conversion() {
        assert_eq!(GamePacketCode::from(0), GamePacketCode::DataAck);
        assert_eq!(GamePacketCode::from(22), GamePacketCode::ChatMessage);
        assert_eq!(GamePacketCode::from(100), GamePacketCode::Ping);
        assert_eq!(GamePacketCode::from(255), GamePacketCode::Unknown);
        assert_eq!(GamePacketCode::from(99), GamePacketCode::Unknown);
    }

    #[test]
    fn test_delivery_mode() {
        assert_eq!(DeliveryMode::Reliable.to_channel(), 0);
        assert_eq!(DeliveryMode::Unreliable.to_channel(), 1);
        assert!(DeliveryMode::Reliable.is_reliable());
        assert!(!DeliveryMode::Unreliable.is_reliable());
    }

    #[test]
    fn test_ping_pong_packets() {
        let ping = create_ping_packet(42, 1);
        assert_eq!(ping.code(), GamePacketCode::Ping);
        assert_eq!(ping.header.sequence, 42);

        let pong = create_pong_packet(42, 2);
        assert_eq!(pong.code(), GamePacketCode::Pong);
        assert_eq!(pong.payload.len(), 4); // u32 sequence
    }

    #[test]
    fn test_chat_packet() {
        let msg = "Hello!";
        let packet = create_chat_packet(1, 0, msg);

        assert_eq!(packet.code(), GamePacketCode::ChatMessage);
        assert_eq!(packet.payload, msg.as_bytes());
    }

    #[test]
    fn test_ffi_header_creation() {
        unsafe {
            let mut buffer = [0u8; 16];
            let mut size = 0i32;

            let result = Platform_Packet_CreateHeader(
                GamePacketCode::Ping as u8,
                123,
                5,
                buffer.as_mut_ptr(),
                &mut size,
            );

            assert_eq!(result, 0);
            assert_eq!(size, GamePacketHeader::SIZE as i32);

            // Validate the header
            assert_eq!(Platform_Packet_ValidateHeader(buffer.as_ptr(), size), 1);

            // Parse the header
            let mut code = 0u8;
            let mut seq = 0u32;
            let mut player = 0u8;

            Platform_Packet_ParseHeader(
                buffer.as_ptr(),
                size,
                &mut code,
                &mut seq,
                &mut player,
            );

            assert_eq!(code, GamePacketCode::Ping as u8);
            assert_eq!(seq, 123);
            assert_eq!(player, 5);
        }
    }

    #[test]
    fn test_ffi_packet_build() {
        unsafe {
            let payload = b"test";
            let mut buffer = [0u8; 64];

            let result = Platform_Packet_Build(
                GamePacketCode::ChatMessage as u8,
                1,
                0,
                payload.as_ptr(),
                payload.len() as i32,
                buffer.as_mut_ptr(),
                buffer.len() as i32,
            );

            assert!(result > 0);
            assert_eq!(result as usize, GamePacketHeader::SIZE + payload.len());

            // Verify payload extraction
            let mut payload_size = 0i32;
            let payload_ptr =
                Platform_Packet_GetPayload(buffer.as_ptr(), result, &mut payload_size);

            assert!(!payload_ptr.is_null());
            assert_eq!(payload_size, payload.len() as i32);

            let extracted = std::slice::from_raw_parts(payload_ptr, payload_size as usize);
            assert_eq!(extracted, payload);
        }
    }
}
