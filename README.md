# QtChatApplication

Chat Application written using C++ & Qt6

## Features
1. Encrypted transport (shared secret, per-message nonce + HMAC)
2. Server-side chat history (JSONL)

## Installation

Download Qt and QtCreator to build application

## How to use
1. build and run `Server`
2. build and run `Client`
3. set the same `CHAT_SHARED_KEY` in the environment for both apps
4. connect client

## Encryption
- Messages and file transfers are encrypted at the application layer.
- The shared key is taken from `CHAT_SHARED_KEY`. If not set, the default fallback key is `CHANGE_ME_SHARED_KEY` (change it).
- This is a lightweight stream-cipher style implementation for demo/learning; do not use as-is for production.

## Server chat history
- The server appends chat messages to `chat_history.jsonl`.
- Location is `QStandardPaths::AppDataLocation` for the server app. If it is unavailable, it falls back to `./history`.
- Each line is a JSON object with fields: `timestamp` (UTC, ISO-8601), `sender`, `receiver`, `message`.
- Only text messages are recorded (file transfers are not logged).
