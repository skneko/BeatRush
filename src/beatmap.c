#include "beatmap.h"

#include <stdio.h>
#include <stdlib.h>

static unsigned int count_lines(FILE *const file) {
	unsigned int count = 0U;

	signed char c;
	while ((c = fgetc(file)) != EOF) {
		if (c == '\n') {
			count++;
		}
	}

	return count;
}

static void load_notes(Beatmap *const map, FILE *const map_file, unsigned int note_count) {
	unsigned long position;
	unsigned char type;
	unsigned char topLane;
	unsigned short duration;

	for (unsigned int i = 0U; i < note_count; i++) {
		if (fscanf(map_file, "%lu\t%hhu\t%hhu\t%hu\n", &position, &type, &topLane, &duration) < 4) {
			printf("Found a bad note definition at %lu.\n", ftell(map_file));
		}

		Note note = { position, type, (bool)topLane, duration, false };
		map->notes[i] = note;
	}
}

#ifdef DEBUG_BEATMAP
void beatmap_print(const Beatmap *const beatmap) {
	printf("BTRM\n\n%d\t%hu\n\n", beatmap->start_offset, beatmap->approach_time);
	for (unsigned int i = 0; i < beatmap->note_count; i++) {
		Note note = beatmap->notes[i];
		printf("%lu\t%hhu\t%hhu\t%hu\n", note.position, note.type, note.topLane, note.duration);
	}
}
#endif

Beatmap *beatmap_load_from_file(const char *const path) {
	FILE *map_file = fopen(path, "r");
	if (map_file == NULL) {
		printf("Error opening beatmap file: %s\n", path);
		return NULL;
	}

	int start_offset;
	unsigned short approach_time;
	if (fscanf(map_file, "BTRM\n\n%d\t%hu\n\n", &start_offset, &approach_time) < 2) {
		printf("File does not look like a valid beatmap: %s", path);
		return NULL;
	}

	long first_note_offset = ftell(map_file);
	unsigned int note_count = count_lines(map_file);

	Beatmap *map = malloc(sizeof(Beatmap));
	map->start_offset = start_offset;
	map->approach_time = approach_time;
	map->note_count = note_count;
	map->notes = malloc(sizeof(Note) * note_count);

	fseek(map_file, first_note_offset, SEEK_SET);
	load_notes(map, map_file, note_count);

	if (fclose(map_file) != 0) {
		printf("Error closing beatmap file: %s\n", path);
		return NULL;
	}

	printf("Loaded beatmap from: %s (%d notes)\n", path, note_count);

#ifdef DEBUG_BEATMAP
	beatmap_print(map);
#endif

	return map;
}