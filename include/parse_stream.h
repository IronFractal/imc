#ifndef PARSE_STREAM_H
#define PARSE_STREAM_H

struct parse_stream;

struct parse_stream *ps_new(const char *filename);

bool ps_is_eof(struct parse_stream *pstream);

char ps_pop(struct parse_stream *pstream);

char ps_peek(struct parse_stream *pstream);

int ps_position(struct parse_stream *pstream);

int ps_column(struct parse_stream *pstream);

int ps_line(struct parse_stream *pstream);

void ps_free(struct parse_stream *pstream);

#endif
