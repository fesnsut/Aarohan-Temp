-- Trading Engine Database Schema

-- Users table
CREATE TABLE IF NOT EXISTS users (
    user_id BIGSERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    email VARCHAR(100) UNIQUE NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- User balances table
CREATE TABLE IF NOT EXISTS user_balances (
    user_id BIGINT PRIMARY KEY REFERENCES users(user_id),
    available_balance BIGINT NOT NULL DEFAULT 0,
    locked_balance BIGINT NOT NULL DEFAULT 0,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Orders table
CREATE TABLE IF NOT EXISTS orders (
    order_id BIGINT PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(user_id),
    symbol VARCHAR(20) NOT NULL,
    side VARCHAR(4) NOT NULL CHECK (side IN ('BUY', 'SELL')),
    order_type VARCHAR(10) NOT NULL CHECK (order_type IN ('LIMIT', 'MARKET')),
    time_in_force VARCHAR(3) NOT NULL CHECK (time_in_force IN ('GFD', 'IOC', 'FOK')),
    price BIGINT NOT NULL,
    quantity BIGINT NOT NULL,
    filled_quantity BIGINT NOT NULL DEFAULT 0,
    status VARCHAR(20) NOT NULL CHECK (status IN ('PENDING', 'PARTIALLY_FILLED', 'FILLED', 'CANCELLED', 'REJECTED')),
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Trades table
CREATE TABLE IF NOT EXISTS trades (
    trade_id BIGINT PRIMARY KEY,
    buy_order_id BIGINT NOT NULL REFERENCES orders(order_id),
    sell_order_id BIGINT NOT NULL REFERENCES orders(order_id),
    buy_user_id BIGINT NOT NULL REFERENCES users(user_id),
    sell_user_id BIGINT NOT NULL REFERENCES users(user_id),
    symbol VARCHAR(20) NOT NULL,
    price BIGINT NOT NULL,
    quantity BIGINT NOT NULL,
    created_at TIMESTAMP NOT NULL
);

-- Market snapshots table (for historical data)
CREATE TABLE IF NOT EXISTS market_snapshots (
    id BIGSERIAL PRIMARY KEY,
    symbol VARCHAR(20) NOT NULL,
    last_trade_price BIGINT NOT NULL,
    bid_price BIGINT,
    bid_quantity BIGINT,
    ask_price BIGINT,
    ask_quantity BIGINT,
    total_volume BIGINT NOT NULL,
    created_at TIMESTAMP NOT NULL
);

-- Indexes for performance
CREATE INDEX IF NOT EXISTS idx_orders_user_id ON orders(user_id);
CREATE INDEX IF NOT EXISTS idx_orders_symbol ON orders(symbol);
CREATE INDEX IF NOT EXISTS idx_orders_status ON orders(status);
CREATE INDEX IF NOT EXISTS idx_orders_created_at ON orders(created_at);

CREATE INDEX IF NOT EXISTS idx_trades_symbol ON trades(symbol);
CREATE INDEX IF NOT EXISTS idx_trades_buy_user ON trades(buy_user_id);
CREATE INDEX IF NOT EXISTS idx_trades_sell_user ON trades(sell_user_id);
CREATE INDEX IF NOT EXISTS idx_trades_created_at ON trades(created_at);

CREATE INDEX IF NOT EXISTS idx_snapshots_symbol ON market_snapshots(symbol);
CREATE INDEX IF NOT EXISTS idx_snapshots_created_at ON market_snapshots(created_at);

-- Insert some test users
INSERT INTO users (user_id, username, email) VALUES
    (1, 'trader1', 'trader1@example.com'),
    (2, 'trader2', 'trader2@example.com'),
    (3, 'trader3', 'trader3@example.com'),
    (4, 'trader4', 'trader4@example.com'),
    (5, 'trader5', 'trader5@example.com'),
    (6, 'trader6', 'trader6@example.com'),
    (7, 'trader7', 'trader7@example.com'),
    (8, 'trader8', 'trader8@example.com'),
    (9, 'trader9', 'trader9@example.com'),
    (10, 'trader10', 'trader10@example.com')
ON CONFLICT (user_id) DO NOTHING;

-- Initialize balances for test users (10,000 USD per user in cents)
INSERT INTO user_balances (user_id, available_balance, locked_balance) VALUES
    (1, 1000000, 0),
    (2, 1000000, 0),
    (3, 1000000, 0),
    (4, 1000000, 0),
    (5, 1000000, 0),
    (6, 1000000, 0),
    (7, 1000000, 0),
    (8, 1000000, 0),
    (9, 1000000, 0),
    (10, 1000000, 0)
ON CONFLICT (user_id) DO NOTHING;
