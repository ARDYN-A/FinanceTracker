CREATE TABLE accounts (
    id          SERIAL PRIMARY KEY,
    name        TEXT NOT NULL,
    type        TEXT CHECK(type IN ('checking','savings','credit','cash')),
    currency    TEXT DEFAULT 'USD',
    created_at  TIMESTAMPTZ DEFAULT now()
);

CREATE TABLE categories (
    id          SERIAL PRIMARY KEY,
    name        TEXT NOT NULL UNIQUE,
    parent_id   INT REFERENCES categories(id),  -- supports subcategories
    color       TEXT  -- hex color for UI charts
);

CREATE TABLE transactions (
    id              SERIAL PRIMARY KEY,
    account_id      INT NOT NULL REFERENCES accounts(id),
    category_id     INT REFERENCES categories(id),
    amount          NUMERIC(12, 2) NOT NULL,  -- never use FLOAT for money
    description     TEXT,
    transaction_date DATE NOT NULL,
    imported_at     TIMESTAMPTZ DEFAULT now(),
    source          TEXT  -- 'manual' or CSV filename
);

CREATE INDEX idx_transactions_date    ON transactions(transaction_date);
CREATE INDEX idx_transactions_account ON transactions(account_id);