package app.djfusion.data

import android.content.ContentValues
import android.content.Context
import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteOpenHelper

data class TrackRow(
    val id: Long,
    val uri: String,
    val title: String,
    val artist: String,
    val durationMs: Long,
)

enum class SortOrder { TITLE, ARTIST, RECENT }

/**
 * Biblioteca local da v1: uma tabela com as faixas que o usuário importou.
 * Só guarda o endereço (content://) de arquivos que o usuário escolheu, nunca cópias dos arquivos.
 * A tabela completa planejada (BPM, tom, cues, playlists) entra nos marcos seguintes, com migração.
 */
class LibraryDb(context: Context) : SQLiteOpenHelper(context, "library.db", null, 1) {

    override fun onCreate(db: SQLiteDatabase) {
        db.execSQL(
            "CREATE TABLE tracks (" +
                "id INTEGER PRIMARY KEY AUTOINCREMENT, " +
                "uri TEXT NOT NULL UNIQUE, " +
                "title TEXT NOT NULL, " +
                "artist TEXT NOT NULL DEFAULT '', " +
                "duration_ms INTEGER NOT NULL, " +
                "created_at INTEGER NOT NULL)",
        )
        db.execSQL("CREATE INDEX idx_tracks_title ON tracks (title)")
        db.execSQL("CREATE INDEX idx_tracks_artist ON tracks (artist)")
    }

    override fun onUpgrade(db: SQLiteDatabase, oldVersion: Int, newVersion: Int) {
        // versão 1: nada a migrar
    }

    /** Retorna true se a faixa é nova (false se o mesmo arquivo já estava na biblioteca). */
    fun insert(uri: String, title: String, artist: String, durationMs: Long): Boolean {
        val values = ContentValues().apply {
            put("uri", uri)
            put("title", title)
            put("artist", artist)
            put("duration_ms", durationMs)
            put("created_at", System.currentTimeMillis())
        }
        return writableDatabase.insertWithOnConflict("tracks", null, values, SQLiteDatabase.CONFLICT_IGNORE) != -1L
    }

    fun delete(id: Long) {
        writableDatabase.delete("tracks", "id = ?", arrayOf(id.toString()))
    }

    fun query(search: String, sort: SortOrder): List<TrackRow> {
        val q = search.trim()
        val where: String?
        val args: Array<String>?
        if (q.isEmpty()) {
            where = null
            args = null
        } else {
            val like = "%" + q.replace("\\", "\\\\").replace("%", "\\%").replace("_", "\\_") + "%"
            where = "title LIKE ? ESCAPE '\\' OR artist LIKE ? ESCAPE '\\'"
            args = arrayOf(like, like)
        }
        val order = when (sort) {
            SortOrder.TITLE -> "title COLLATE NOCASE ASC, id ASC"
            SortOrder.ARTIST -> "artist COLLATE NOCASE ASC, title COLLATE NOCASE ASC, id ASC"
            SortOrder.RECENT -> "created_at DESC, id DESC"
        }
        val out = ArrayList<TrackRow>()
        readableDatabase.query(
            "tracks", arrayOf("id", "uri", "title", "artist", "duration_ms"), where, args, null, null, order,
        ).use { c ->
            while (c.moveToNext()) {
                out.add(TrackRow(c.getLong(0), c.getString(1), c.getString(2), c.getString(3), c.getLong(4)))
            }
        }
        return out
    }
}
