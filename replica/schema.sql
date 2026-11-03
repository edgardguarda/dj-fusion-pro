-- DJ FUSION PRO: esquema local (Room / SQLite)
-- Sem servidor e sem conta de usuário na v1: o dono dos dados é o aparelho.
-- Tempos de posição em milissegundos inteiros. Datas em milissegundos desde 1970 (UTC).
PRAGMA foreign_keys = ON;

CREATE TABLE tracks (
  id              INTEGER PRIMARY KEY AUTOINCREMENT,
  uri             TEXT    NOT NULL UNIQUE,           -- content:// autorizado pelo seletor de arquivos
  title           TEXT    NOT NULL,
  artist          TEXT,
  album           TEXT,
  duration_ms     INTEGER NOT NULL CHECK (duration_ms > 0),
  sample_rate     INTEGER,
  channels        INTEGER CHECK (channels IN (1, 2)),
  mime            TEXT,
  bpm             REAL    CHECK (bpm IS NULL OR bpm BETWEEN 40 AND 250),
  first_beat_ms   REAL,
  key_camelot     TEXT    CHECK (key_camelot IS NULL OR key_camelot GLOB '[0-9]*[AB]'),
  intro_end_ms    INTEGER,
  outro_start_ms  INTEGER,
  analysis_state  TEXT    NOT NULL DEFAULT 'pending'
                  CHECK (analysis_state IN ('pending', 'running', 'done', 'failed')),
  cache_path      TEXT,                              -- PCM decodificado no cache do app
  created_at      INTEGER NOT NULL,
  updated_at      INTEGER NOT NULL,
  CHECK (outro_start_ms IS NULL OR intro_end_ms IS NULL OR outro_start_ms >= intro_end_ms)
);
CREATE INDEX idx_tracks_title  ON tracks (title);
CREATE INDEX idx_tracks_artist ON tracks (artist);
CREATE INDEX idx_tracks_bpm    ON tracks (bpm);
CREATE INDEX idx_tracks_key    ON tracks (key_camelot);

CREATE TABLE cues (
  id          INTEGER PRIMARY KEY AUTOINCREMENT,
  track_id    INTEGER NOT NULL REFERENCES tracks(id) ON DELETE CASCADE,
  kind        TEXT    NOT NULL CHECK (kind IN ('cue', 'hotcue', 'loop')),
  slot        INTEGER NOT NULL DEFAULT 0 CHECK (slot BETWEEN 0 AND 15),
  position_ms INTEGER NOT NULL CHECK (position_ms >= 0),
  end_ms      INTEGER,                               -- só para loops
  label       TEXT,
  color       INTEGER,
  created_at  INTEGER NOT NULL,
  updated_at  INTEGER NOT NULL,
  UNIQUE (track_id, kind, slot),
  CHECK (end_ms IS NULL OR end_ms > position_ms),
  CHECK (kind <> 'loop' OR end_ms IS NOT NULL)
);
CREATE INDEX idx_cues_track ON cues (track_id);

CREATE TABLE playlists (
  id         INTEGER PRIMARY KEY AUTOINCREMENT,
  name       TEXT    NOT NULL,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL
);

CREATE TABLE playlist_items (
  id          INTEGER PRIMARY KEY AUTOINCREMENT,
  playlist_id INTEGER NOT NULL REFERENCES playlists(id) ON DELETE CASCADE,
  track_id    INTEGER NOT NULL REFERENCES tracks(id)    ON DELETE CASCADE,
  position    INTEGER NOT NULL CHECK (position >= 0),
  UNIQUE (playlist_id, position)
);
CREATE INDEX idx_playlist_items_track ON playlist_items (track_id);

-- Studio Timeline (fase 5)
CREATE TABLE projects (
  id          INTEGER PRIMARY KEY AUTOINCREMENT,
  name        TEXT    NOT NULL,
  master_bpm  REAL    CHECK (master_bpm IS NULL OR master_bpm BETWEEN 40 AND 250),
  sample_rate INTEGER NOT NULL DEFAULT 48000,
  created_at  INTEGER NOT NULL,
  updated_at  INTEGER NOT NULL
);

CREATE TABLE lanes (
  id         INTEGER PRIMARY KEY AUTOINCREMENT,
  project_id INTEGER NOT NULL REFERENCES projects(id) ON DELETE CASCADE,
  lane_index INTEGER NOT NULL CHECK (lane_index >= 0),
  name       TEXT,
  UNIQUE (project_id, lane_index)
);

CREATE TABLE clips (
  id             INTEGER PRIMARY KEY AUTOINCREMENT,
  lane_id        INTEGER NOT NULL REFERENCES lanes(id)  ON DELETE CASCADE,
  track_id       INTEGER NOT NULL REFERENCES tracks(id) ON DELETE RESTRICT,
  start_ms       INTEGER NOT NULL CHECK (start_ms >= 0),  -- posição na timeline
  source_in_ms   INTEGER NOT NULL CHECK (source_in_ms >= 0),
  source_out_ms  INTEGER NOT NULL,
  gain_db        REAL    NOT NULL DEFAULT 0,
  stretch_ratio  REAL    NOT NULL DEFAULT 1.0 CHECK (stretch_ratio BETWEEN 0.5 AND 2.0),
  CHECK (source_out_ms > source_in_ms)
);
CREATE INDEX idx_clips_lane  ON clips (lane_id, start_ms);
CREATE INDEX idx_clips_track ON clips (track_id);

CREATE TABLE automation_points (
  id      INTEGER PRIMARY KEY AUTOINCREMENT,
  clip_id INTEGER NOT NULL REFERENCES clips(id) ON DELETE CASCADE,
  target  TEXT    NOT NULL CHECK (target IN
            ('volume', 'eq_low', 'eq_mid', 'eq_high', 'filter', 'fx_mix', 'tempo')),
  time_ms INTEGER NOT NULL CHECK (time_ms >= 0),      -- relativo ao início do clip
  value   REAL    NOT NULL,
  curve   TEXT    NOT NULL DEFAULT 'linear' CHECK (curve IN ('linear', 'step', 'smooth')),
  UNIQUE (clip_id, target, time_ms)
);
CREATE INDEX idx_automation_clip ON automation_points (clip_id, target, time_ms);

-- Controladoras (fase 3). O JSON é gerado pelo MIDI Learn, nada copiado de terceiros.
CREATE TABLE controller_profiles (
  id          INTEGER PRIMARY KEY AUTOINCREMENT,
  name        TEXT    NOT NULL,
  usb_vendor  INTEGER,
  usb_product INTEGER,
  mapping_json TEXT   NOT NULL,
  created_at  INTEGER NOT NULL,
  updated_at  INTEGER NOT NULL,
  UNIQUE (usb_vendor, usb_product, name)
);
