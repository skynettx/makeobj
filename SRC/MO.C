/*
** makeobj.c
**
**---------------------------------------------------------------------------
** Copyright 2014 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
** This is a throwaway program to create OMF object files for DOS. It also
** extracts the object files.  It should be compatible with MakeOBJ by John
** Romero except where we calculate the checksum correctly.
**
*/

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


#pragma pack(1)
typedef struct
{
	unsigned char type;
	unsigned short len;
} SegHeader;

typedef struct
{
	unsigned short len;
	unsigned char name;
	unsigned char classname;
	unsigned char overlayname;
} SegDef;
#pragma pack()

const char* ReadFile(const char* fn, long* size)
{
	char* out;

	FILE* f = fopen(fn, "rb");

	if (f == NULL)
		return 0;

	fseek(f, 0, SEEK_END);
	*size = ftell(f);
	out = (char*)malloc(*size);
	fseek(f, 0, SEEK_SET);

	fread(out, *size, 1, f);

	fclose(f);

	return out;
}

long ReadFileSize(const char* fn, long size)
{
	FILE* f = fopen(fn, "rb");

	if (f == NULL)
		return 0;

	fseek(f, 0, SEEK_END);

	size = ftell(f);

	fclose(f);

	return size;
}

void WriteFile(const char* fn, const char* data, long size)
{
	FILE* f = fopen(fn, "wb");
	fwrite(data, size, 1, f);
	fclose(f);
}

char* RemoveExt(char* myStr)
{
	char* retStr;
	char* lastExt;

	if (myStr == NULL)
		return NULL;

	if ((retStr = malloc(strlen(myStr) + 1)) == NULL)
		return NULL;

	strcpy(retStr, myStr);
	lastExt = strrchr(retStr, '.');

	if (lastExt != NULL)
		*lastExt = '\0';

	return retStr;
}

char* RemovePathFromString(char* p)
{
	char* fn;
	char* input;

	input = p;

	if (input[(strlen(input) - 1)] == '/' || input[(strlen(input) - 1)] == '\\')
		input[(strlen(input) - 1)] = '\0';

	if (strchr(input, '\\'))
		(fn = strrchr(input, '\\')) ? ++fn : (fn = input);
	else
		(fn = strrchr(input, '/')) ? ++fn : (fn = input);

	return fn;
}

void Extract(const char* infn)
{
	const char* in;
	const char* start;
	const char* p;
	char outfn[16];
	char str[256];
	char upinfn[260];
	char* outdata;
	long outsize;
	long insize;
	int i;
	SegHeader head;

	outdata = NULL;

	strcpy(upinfn, infn);

	for (i = 0; i < strlen(upinfn); ++i)
		upinfn[i] = toupper(upinfn[i]);

	insize = ReadFileSize(infn, insize);

	if (insize > 0x753B)
	{
		printf("Can't convert '%s  ' because it is over 29K in size\n", upinfn);
		return;
	}

	start = in = ReadFile(infn, &insize);

	if (in == NULL)
		return;

	while (in < start + insize)
	{
		head = *(SegHeader*)in;

		switch (head.type)
		{
		case 0x80: /* THEADR */
			memcpy(outfn, in + 4, in[3]);
			outfn[in[3]] = 0;
			printf("Output: %s\n", outfn);
			{
				for (i = 0; i < 16; ++i)
				{
					if (outfn[i] == ' ')
						outfn[i] = 0;
				}
			}
			break;
		case 0x88: /* COMENT */
			switch (in[3])
			{
			case 0:
				memcpy(str, in + 5, head.len - 2);
				str[head.len - 3] = 0;
				printf("Comment: %s\n", str);
				break;
			default:
				printf("Unknown comment type %X @ %x ignored.\n", (unsigned char)in[3], (unsigned int)(in - start));
				break;
			}
			break;
		case 0x96: /* LNAMES */
			p = in + 3;
			while (p < in + head.len + 2)
			{
				memcpy(str, p + 1, (unsigned char)*p);
				str[(unsigned char)*p] = 0;
				printf("Name: %s\n", str);

				p += (unsigned char)*p + 1;
			}
			break;
		case 0x98: /* SEGDEF */
		{
			SegDef* sd;

			sd = *(in + 3) ? (SegDef*)(in + 4) : (SegDef*)(in + 7);
			printf("Segment Length: %d\n", sd->len);

			outdata = (char*)malloc(sd->len);
			outsize = sd->len;
			break;
		}
		case 0x90: /* PUBDEF */
			p = in + 5;
			if (in[5] == 0)
				p += 2;
			while (p < in + head.len + 2)
			{
				memcpy(str, p + 1, (unsigned char)*p);
				str[(unsigned char)*p] = 0;
				printf("Public Name: %s\n", str);

				p += (unsigned char)*p + 4;
			}
			break;
		case 0xA0: /* LEDATA */
			printf("Writing data at %d (%d)\n", *(unsigned short*)(in + 4), head.len - 4);
			memcpy(outdata + *(unsigned short*)(in + 4), in + 6, head.len - 4);
			break;
		case 0x8A: /* MODEND */
			/* Ignore */
			break;
		default:
			printf("Unknown header type %X @ %x ignored.\n", head.type, (unsigned int)(in - start));
			break;
		}

		in += 3 + head.len;
	}

	WriteFile(outfn, outdata, outsize);

	free((char*)start);
	free(outdata);
}

void CheckSum(char* s, unsigned short len)
{
	int sum;

	len += 3;

	sum = 0;
	while (len > 1)
	{
		sum += *(unsigned char*)s;
		++s;
		--len;
	}
	*s = (unsigned char)(0x100 - (sum & 0xFF));
}

void MakeDataObj(const char* infn, const char* outfn, const char* segname, const char* symname, int altmode)
{
#define Flush() fwrite(d.buf, d.head.len+3, 1, f)
	union
	{
		char buf[4096];
		SegHeader head;
	} d;
	long i;
	long j;
	FILE* f;
	long insize;
	long segsize;
	char pubname[260];
	char upinfn[260];
	char upoutfn[260];
	const char* pub;
	const char* in;
	const char* inseg;
	const char* infn_stripped = strrchr(infn, '/');
	if (strrchr(infn, '\\') > infn_stripped)
		infn_stripped = strrchr(infn, '\\');
	if (infn_stripped == NULL)
		infn_stripped = infn;
	else
		++infn_stripped;

	strcpy(upinfn, infn);

	for (i = 0; i < strlen(upinfn); ++i)
		upinfn[i] = toupper(upinfn[i]);

	strcpy(upoutfn, outfn);

	for (i = 0; i < strlen(upoutfn); ++i)
		upoutfn[i] = toupper(upoutfn[i]);

	insize = ReadFileSize(infn, insize);

	if (insize > 0x20000)
	{
		printf("Can't convert '%s  ' because it is over 128K in size\n", upinfn);
		return;
	}

	if (access(infn, 0))
		return;

	f = fopen(outfn, "wb");

	d.head.type = 0x80;
	d.head.len = 14;
	d.buf[3] = 12;
	if (d.buf[3] > 12)
		d.buf[3] = 12;
	sprintf(&d.buf[4], "%-12s", infn_stripped);
	for (i = 0; i < strlen(infn_stripped) && i < 12; ++i)
		d.buf[4 + i] = toupper(d.buf[4 + i]);
	/* CheckSum(d.buf, d.head.len); */
	d.buf[17] = 0; /* For some reason this one isn't checksummed by MakeOBJ */
	Flush();

	d.head.type = 0x88;
	d.head.len = 15;
	d.buf[3] = 0;
	d.buf[4] = 0;
	/* We're not really MakeOBJ v1.1, but to allow us to verify with md5sums */
	memcpy(&d.buf[5], "MakeOBJ v1.2", 12);
	CheckSum(d.buf, d.head.len);
	Flush();

	d.head.type = 0x96;
	d.head.len = strlen(infn_stripped) + 40;
	d.buf[3] = 6;
	memcpy(&d.buf[4], "DGROUP", 6);
	d.buf[10] = 5;
	memcpy(&d.buf[11], "_DATA", 5);
	d.buf[16] = 4;
	memcpy(&d.buf[17], "DATA", 4);
	d.buf[21] = 0;
	d.buf[22] = 5;
	memcpy(&d.buf[23], "_TEXT", 5);
	d.buf[28] = 4;
	memcpy(&d.buf[29], "CODE", 4);
	d.buf[33] = 8;
	memcpy(&d.buf[34], "FAR_DATA", 8);
	if (!segname)
	{
		if (!altmode)
		{
			d.buf[42] = strlen(infn_stripped) - 1;
			for (i = 0; i < strlen(infn_stripped) - 4; ++i)
			{
				if (i == 0)
					d.buf[43] = toupper(infn_stripped[0]);
				else
					d.buf[43 + i] = tolower(infn_stripped[i]);
			}
			memcpy(&d.buf[43 + i], "Seg", 3);
		}
		else
		{
			d.head.len = 40;
		}
	}
	else
	{
		d.head.len = strlen(segname) + 41;
		d.buf[42] = strlen(segname);
		strcpy(&d.buf[43], segname);
	}
	CheckSum(d.buf, d.head.len);
	Flush();

	d.head.type = 0x98;
	d.head.len = 7;
	*(unsigned short*)(d.buf + 4) = insize;
	if (altmode == 0)
	{
		d.buf[3] = (char)((unsigned char)0x60);
		d.buf[6] = 8;
		d.buf[7] = 7;
		d.buf[8] = 4;
	}
	else
	{
		d.buf[3] = (char)((unsigned char)0x48);
		d.buf[6] = 2;
		d.buf[7] = 3;
		d.buf[8] = 4;
	}
	CheckSum(d.buf, d.head.len);
	Flush();

	if (altmode)
	{
		d.head.type = 0x9A;
		d.head.len = 4;
		d.buf[3] = 1;
		d.buf[4] = (char)((unsigned char)0xFF);
		d.buf[5] = 1;
		CheckSum(d.buf, d.head.len);
		Flush();
	}

	d.head.type = 0x90;
	d.head.len = strlen(infn_stripped) + 4;
	d.buf[3] = 1;
	d.buf[4] = 1;
	if (!symname)
	{
		d.buf[5] = strlen(infn_stripped) - 3;
		d.buf[6] = '_';
		for (i = 0; i < strlen(infn_stripped) - 4; ++i)
			d.buf[7 + i] = tolower(infn_stripped[i]);
	}
	else
	{
		d.head.len = strlen(symname) + 7;
		d.buf[5] = strlen(symname);
		strcpy(&d.buf[6], symname);
		i = strlen(symname) - 1;
	}
	d.buf[7 + i] = 0;
	d.buf[8 + i] = 0;
	d.buf[9 + i] = 0;
	/* This checksum is calculated wrong in MakeOBJ, although I don't know in what way. */
	CheckSum(d.buf, d.head.len);
	Flush();

#define LEDATA_LEN 1024

	segsize = insize;

	if (segsize > 0x7fff)
	{
		FILE* finseg = fopen(infn, "rb");
		inseg = (char*)malloc(0x7fff);

		while (segsize > 0x7fff)
		{
			fread(inseg, 0x7fff, 1, finseg);

			for (j = 0; j < 0x7fff; j += LEDATA_LEN)
			{
				d.head.type = 0xA0;
				d.head.len = 0x7fff - j > LEDATA_LEN ? LEDATA_LEN + 4 : 0x7fff - j + 4;
				d.buf[3] = 1;
				*(unsigned short*)(d.buf + 4) = j;
				memcpy(&d.buf[6], &inseg[j], d.head.len - 4);
				CheckSum(d.buf, d.head.len);
				Flush();
			}

			segsize -= 0x7fff;
		}

		fread(inseg, segsize, 1, finseg);

		for (j = 0; j < segsize; j += LEDATA_LEN)
		{
			d.head.type = 0xA0;
			d.head.len = segsize - j > LEDATA_LEN ? LEDATA_LEN + 4 : segsize - j + 4;
			d.buf[3] = 1;
			*(unsigned short*)(d.buf + 4) = j;
			memcpy(&d.buf[6], &inseg[j], d.head.len - 4);
			CheckSum(d.buf, d.head.len);
			Flush();
		}

		fclose(finseg);
		free((char*)inseg);
	}
	else
	{
		in = ReadFile(infn, &insize);

		for (j = 0; j < insize; j += LEDATA_LEN)
		{
			d.head.type = 0xA0;
			d.head.len = insize - j > LEDATA_LEN ? LEDATA_LEN + 4 : insize - j + 4;
			d.buf[3] = 1;
			*(unsigned short*)(d.buf + 4) = j;
			memcpy(&d.buf[6], &in[j], d.head.len - 4);
			CheckSum(d.buf, d.head.len);
			Flush();
		}

		free((char*)in);
	}

	d.head.type = 0x8A;
	d.head.len = 2;
	d.buf[3] = 0;
	d.buf[4] = 0;
	CheckSum(d.buf, d.head.len);
	Flush();

	if (segname)
	{
		strcpy(pubname, segname);
		pub = "PUBLIC:";
	}
	else if (symname)
	{
		strcpy(pubname, symname);
		pub = "PUBLIC:";
	}
	else
	{
		strcpy(pubname, infn_stripped);
		strcpy(pubname, RemoveExt(pubname));

		for (i = 0; i < strlen(pubname); ++i)
			pubname[i] = tolower(pubname[i]);

		pub = "PUBLIC:_";
	}

	if (insize < 100)
		printf("%-15s %s%-16sSIZE:%ld Saved as %s\n", upinfn, pub, pubname, insize, upoutfn);
	else
		printf("%-15s %s%-16sSIZE:%-10ld Saved as %s\n", upinfn, pub, pubname, insize, upoutfn);

	fclose(f);
}

void DumpData(const char* infn, const char* outfn, int skip)
{
	FILE* f;
	long i;
	long insize;
	char symname[9];
	char symnameout[12];
	char upinfn[260];
	char upoutfn[260];
	const char* in;
	const char* infn_stripped = strrchr(infn, '/');
	if (strrchr(infn, '\\') > infn_stripped)
		infn_stripped = strrchr(infn, '\\');
	if (infn_stripped == NULL)
		infn_stripped = infn;
	else
		++infn_stripped;

	strcpy(upinfn, infn);

	for (i = 0; i < strlen(upinfn); ++i)
		upinfn[i] = toupper(upinfn[i]);

	strcpy(upoutfn, outfn);

	for (i = 0; i < strlen(upoutfn); ++i)
		upoutfn[i] = toupper(upoutfn[i]);

	in = ReadFile(infn, &insize);

	if (in == NULL)
		return;

	f = fopen(outfn, "wb");

	memset(symname, 0, 9);
	memcpy(symname, infn_stripped, strlen(infn_stripped) - 4);
	fprintf(f, "char far %s[] ={\r\n", symname);

	for (i = skip; i < insize; ++i)
	{
		fprintf(f, "%d", (unsigned char)in[i]);
		if (i != insize - 1L)
			fprintf(f, ",\r\n");
	}

	fprintf(f, " };\r\n");

	sprintf(symnameout, "%s[]", symname);
	printf("%-15s char far %-20s Saved as %s\n", upinfn, symnameout, upoutfn);

	fclose(f);
	free((char*)in);
}

int main(int argc, char* argv[])
{
	char defoutfn[260];
	char buf[260];
	int skip = 0;
	int i;

	printf("MakeOBJ v0.8\n"
		"\n");

	if (argc < 3)
	{
		printf("MakeOBJ will take your files and convert them into the Microsoft Object\n"
			"Module Format so you can link data directly into your program and save\n"
			"disk space, especially if you compress your EXE file! Remember that any\n"
			"data you link in will REMAIN in memory for the entire time your program\n"
			"is executing, so if your heap space is looking tight, this might not be\n"
			"a good idea.\n"
			"\n"
			"Command-Line options:\n"
			"-F      Converts all files to FARDATA segments\n"
			"-C      Converts all files to CODE segments\n"
			"-X      Converts all OMF files to ORIGINAL files\n"
			"-S      Converts all files to CHAR arrays\n"
			"\n"
			"To invoke MakeOBJ, merely type MO <filename> <options>. You may use the\n"
			"wildcard characters in the filename to convert a bunch of files.\n");

		return 0;
	}

	for (i = 0; i < argc; i++)
	{
		strcpy(buf, RemovePathFromString(argv[i]));

		if (strlen(buf) > 12)
		{
			printf("Inputfilename %s to long max 8 Characters\n", buf);
			printf("MakeOBJ finished.\n");
			return 0;
		}
	}

	if ((strcmp(argv[1], "-f") == 0) || (strcmp(argv[1], "-F") == 0) ||
		(strcmp(argv[argc - 1], "-f") == 0) || (strcmp(argv[argc - 1], "-F") == 0))
	{
		if ((strcmp(argv[argc - 1], "-f") == 0) || (strcmp(argv[argc - 1], "-F") == 0))
		{
			for (i = 1; i < argc - 1; i++)
			{
				strcpy(buf, argv[i]);

				strcpy(buf, RemoveExt(buf));
				strcpy(defoutfn, buf);
				sprintf(defoutfn, "%s.obj", buf);

				MakeDataObj(argv[i], defoutfn, NULL, NULL, 0);
			}
		}
		else
		{
			for (i = 2; i < argc; i++)
			{
				strcpy(buf, argv[i]);

				strcpy(buf, RemoveExt(buf));
				strcpy(defoutfn, buf);
				sprintf(defoutfn, "%s.obj", buf);

				MakeDataObj(argv[i], defoutfn, NULL, NULL, 0);
			}
		}
	}
	else if ((strcmp(argv[1], "-x") == 0) || (strcmp(argv[1], "-X") == 0) ||
		(strcmp(argv[argc - 1], "-x") == 0) || (strcmp(argv[argc - 1], "-X") == 0))
	{
		if ((strcmp(argv[argc - 1], "-x") == 0) || (strcmp(argv[argc - 1], "-X") == 0))
		{
			for (i = 1; i < argc - 1; i++)
			{
				Extract(argv[i]);
			}
		}
		else
		{
			for (i = 2; i < argc; i++)
			{
				Extract(argv[i]);
			}
		}
	}
	else if ((strcmp(argv[1], "-s") == 0) || (strcmp(argv[1], "-S") == 0) ||
		(strcmp(argv[argc - 1], "-s") == 0) || (strcmp(argv[argc - 1], "-S") == 0))
	{
		if ((strcmp(argv[argc - 1], "-s") == 0) || (strcmp(argv[argc - 1], "-S") == 0))
		{
			for (i = 1; i < argc - 1; i++)
			{
				strcpy(buf, argv[i]);
				strcpy(buf, RemoveExt(buf));
				strcpy(defoutfn, buf);
				sprintf(defoutfn, "%s.h", buf);

				DumpData(argv[i], defoutfn, skip);
			}
		}
		else
		{
			for (i = 2; i < argc; i++)
			{
				strcpy(buf, argv[i]);
				strcpy(buf, RemoveExt(buf));
				strcpy(defoutfn, buf);
				sprintf(defoutfn, "%s.h", buf);

				DumpData(argv[i], defoutfn, skip);
			}
		}
	}
	else
	{
		if ((strcmp(argv[argc - 1], "-c") == 0) || (strcmp(argv[argc - 1], "-C") == 0))
		{
			for (i = 1; i < argc - 1; i++)
			{
				strcpy(buf, argv[i]);

				strcpy(buf, RemoveExt(buf));
				strcpy(defoutfn, buf);
				sprintf(defoutfn, "%s.obj", buf);

				MakeDataObj(argv[i], defoutfn, NULL, NULL, 1);
			}
		}
		else if (access(argv[argc - 1], 0))
		{
			printf("MakeOBJ finished.\n");
			return 0;
		}
		else
		{
			for (i = 2; i < argc; i++)
			{
				strcpy(buf, argv[i]);

				strcpy(buf, RemoveExt(buf));
				strcpy(defoutfn, buf);
				sprintf(defoutfn, "%s.obj", buf);

				MakeDataObj(argv[i], defoutfn, NULL, NULL, 1);
			}
		}
	}

	printf("MakeOBJ finished.\n");

	return 0;
}