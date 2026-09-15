/* drive_json.c — minimal json-c harness for ECE 4974 Lab 1
 *
 * Reads a file, feeds the bytes through json-c's incremental tokener,
 * then exercises the resulting object a little (serialize + a few
 * accessor paths) so bugs in those paths also get reached.
 *
 * Build:
 *   clang -fsanitize=address,undefined -fno-omit-frame-pointer \
 *         -fno-optimize-sibling-calls -g -O1 \
 *         -I<build>/include -I<src> drive_json.c <build>/libjson-c.a \
 *         -o drive_json
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json.h>

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "usage: %s <file.json>\n", argv[0]);
		return 2;
	}

	FILE *f = fopen(argv[1], "rb");
	if (!f)
	{
		perror("fopen");
		return 2;
	}
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (n < 0)
	{
		fclose(f);
		return 2;
	}

	char *buf = malloc((size_t)n + 1);
	if (!buf)
	{
		fclose(f);
		return 2;
	}
	size_t got = fread(buf, 1, (size_t)n, f);
	buf[got] = '\0';
	fclose(f);

	/* Incremental parse: mirrors how a network service would feed bytes in. */
	struct json_tokener *tok = json_tokener_new();
	if (!tok)
	{
		free(buf);
		return 2;
	}

	struct json_object *obj = json_tokener_parse_ex(tok, buf, (int)got);
	enum json_tokener_error err = json_tokener_get_error(tok);

	if (obj == NULL)
	{
		fprintf(stderr, "parse failed: %s\n", json_tokener_error_desc(err));
	}
	else
	{
		/* Touch the common accessor paths so their bugs are reachable too. */
		const char *s = json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PLAIN);
		if (s)
			fprintf(stdout, "len=%zu\n", strlen(s));

		(void)json_object_get_int64(obj);
		(void)json_object_get_double(obj);
		(void)json_object_get_boolean(obj);
		if (json_object_is_type(obj, json_type_array))
		{
			size_t len = json_object_array_length(obj);
			for (size_t i = 0; i < len && i < 64; i++)
				(void)json_object_array_get_idx(obj, i);
		}
		json_object_put(obj);
	}

	json_tokener_free(tok);
	free(buf);
	return obj ? 0 : 1;
}
