/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2003                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function: tool for adding comments to Ogg Theora files
  last mod: $Id$

 ********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <assert.h>
#include <getopt.h>

#if HAVE_STAT && HAVE_CHMOD
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "utf8.h"

#define _(str) str

#include "commenter.h"

/* getopt format struct */
struct option long_options[] = {
	{"list",0,0,'l'},
	{"append",0,0,'a'},
	{"tag",0,0,'t'},
	{"write",0,0,'w'},
	{"help",0,0,'h'},
	{"quiet",0,0,'q'}, /* unused */
	{"version", 0, 0, 'V'},
	{"commentfile",1,0,'c'},
	{"raw", 0,0,'R'},
	{NULL,0,0,0}
};

/* local parameter storage from parsed options */
typedef struct {
	/* mode and flags */
	int	mode;
	int	raw;

	/* file names and handles */
	char	*infilename, *outfilename;
	char	*commentfilename;
	FILE	*commentfile;
	int	tempoutfile;

	/* comments */
	int	commentcount;
	char	**comments;
} param_t;

#define MODE_NONE  0
#define MODE_LIST  1
#define MODE_WRITE 2
#define MODE_APPEND 3

/* prototypes */
void usage(void);
void print_comments(FILE *out, TheoraCommenter* com, int raw);
int  add_comment(char *line, TheoraCommenter* com, int raw);

param_t	*new_param(void);
void free_param(param_t *param);
void parse_options(int argc, char *argv[], param_t *param);
void open_files(param_t *p);
void close_files(param_t *p, int output_written);

/** The input file */
static FILE* inputFile;
/** The output file */
static FILE* outputFile;

/**
 * The input for the commenter, reading the input file.
 * @see TheoraCommenterInput
 */
size_t inputFunc(char* buf, size_t n)
{
  assert(inputFile);
  return fread(buf, 1, n, inputFile);
}

/**
 * The output for the commenter, writing to the output file.
 * @see TheoraCommenterOutput
 */
size_t outputFunc(char* buf, size_t n)
{
  assert(outputFile);
  return fwrite(buf, 1, n, outputFile);
}

/**********
   main.c

   This is the main function where options are read and written
   you should be able to just read this function and see how
   to call the vcedit routines. Details of how to pack/unpack the
   vorbis_comment structure itself are in the following two routines.
   The rest of the file is ui dressing so make the program minimally
   useful as a command line utility and can generally be ignored.

***********/

int main(int argc, char **argv)
{
	param_t	*param;
	int i;
  TheoraCommenter* com;

  /*
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
  */

	/* initialize the cmdline interface */
	param = new_param();
	parse_options(argc, argv, param);

	/* take care of opening the requested files */
	/* relevent file pointers are returned in the param struct */
	open_files(param);

  #define FINALLY \
    { \
      deleteTheoraCommenter(com); \
      close_files(param, 0); \
      free_param(param); \
    }

  /* Create the commenter */
  com=newTheoraCommenter(&inputFunc, &outputFunc);
  if(theoraCommenter_read(com))
  {
    fprintf(stderr, "%s\n", com->error);
    FINALLY
    return EXIT_FAILURE;
  }

	/* which mode are we in? */

	if (param->mode == MODE_LIST) {
		print_comments(param->commentfile, com, param->raw);
    FINALLY
		return 0;		
	}

	if (param->mode == MODE_WRITE || param->mode == MODE_APPEND)
  {
    /* Clear comments when not appending */
    if(param->mode!=MODE_APPEND)
      theoraCommenter_clear(com);
    
    /* Add the comments */
		for(i=0; i!=param->commentcount; ++i)
		{
			if(add_comment(param->comments[i], com, param->raw))
				fprintf(stderr, _("Bad comment: \"%s\"\n"), param->comments[i]);
		}

    /* Seek input to beginning */
    fseek(inputFile, 0, SEEK_SET);

    /* Write out */
    if(theoraCommenter_write(com))
    {
      fprintf(stderr, "%s\n", com->error);
      FINALLY
      return EXIT_FAILURE;
    }

    FINALLY
    return EXIT_SUCCESS;
	}

	/* should never reach this point */
	fprintf(stderr, _("no action specified\n"));
  FINALLY
	return 1;
}

/**********

   Print out the comments from the vorbis structure

   this version just dumps the raw strings
   a more elegant version would use vorbis_comment_query()

***********/

void print_comments(FILE *out, TheoraCommenter* com, int raw)
{
	int i;
	char *decoded_value;

	for(i=0; i!=com->comments.comments; ++i)
		if(!raw && utf8_decode(com->comments.user_comments[i], &decoded_value)>=0)
    {
   		fprintf(out, "%s\n", decoded_value);
			free(decoded_value);
		} else
    {
			fprintf(out, "%s\n", com->comments.user_comments[i]);
		}
}

/**********

   Take a line of the form "TAG=value string", parse it, convert the
   value to UTF-8, and add it to the
   vorbis_comment structure. Error checking is performed.

   Note that this assumes a null-terminated string, which may cause
   problems with > 8-bit character sets!

***********/

int add_comment(char *line, TheoraCommenter* com, int raw)
{
	char	*mark, *value, *utf8_value;

	/* strip any terminal newline */
	{
		int len = strlen(line);
		if (line[len-1] == '\n') line[len-1] = '\0';
	}

	/* validation: basically, we assume it's a tag
	 * if it has an '=' after one or more valid characters,
	 * as the comment spec requires. For the moment, we
	 * also restrict ourselves to 0-terminated values */

	mark = strchr(line, '=');
	if (mark == NULL) return -1;

	value = line;
	while (value < mark) {
		if(*value < 0x20 || *value > 0x7d || *value == 0x3d) return -1;
		value++;
	}

	/* split the line by turning the '=' in to a null */
	*mark = '\0';	
	value++;

  if(raw)
  {
    theoraCommenter_addTag(com, line, value);
    return 0;
  }
	/* convert the value from the native charset to UTF-8 */
  else if (utf8_encode(value, &utf8_value) >= 0)
  {
		/* append the comment and return */
    theoraCommenter_addTag(com, line, utf8_value);
    free(utf8_value);
		return 0;
	} else
  {
		fprintf(stderr, _("Couldn't convert comment to UTF-8, "
			"cannot add\n"));
		return -1;
	}
}


/*** ui-specific routines ***/

/**********

   Print out to usage summary for the cmdline interface (ui)

***********/

/* XXX: -q is unused
  printf (_("  -q, --quiet             Don't display comments while editing\n"));
*/

void usage(void)
{

  printf (_("theoracomment from %s %s\n"
            " by the Xiph.Org Foundation (http://www.xiph.org/)\n\n"), PACKAGE, VERSION);

  printf (_("List or edit comments in Ogg Vorbis files.\n"));
  printf ("\n");

  printf (_("Usage: \n"
            "  theoracomment [-Vh]\n" 
            "  theoracomment [-lR] file\n"
            "  theoracomment [-R] [-c file] [-t tag] <-a|-w> inputfile [outputfile]\n"));
  printf ("\n");

  printf (_("Listing options\n"));
  printf (_("  -l, --list              List the comments (default if no options are given)\n"));
  printf ("\n");

  printf (_("Editing options\n"));
  printf (_("  -a, --append            Append comments\n"));
  printf (_("  -t \"name=value\", --tag \"name=value\"\n"
            "                          Specify a comment tag on the commandline\n"));
  printf (_("  -w, --write             Write comments, replacing the existing ones\n"));
  printf ("\n");

  printf (_("Miscellaneous options\n"));
  printf (_("  -c file, --commentfile file\n"
            "                          When listing, write comments to the specified file.\n"
            "                          When editing, read comments from the specified file.\n"));
  printf (_("  -R, --raw               Read and write comments in UTF-8\n"));
  printf ("\n");

  printf (_("  -h, --help              Display this help\n"));
  printf (_("  -V, --version           Output version information and exit\n"));
  printf ("\n");

  printf (_("If no output file is specified, theoracomment will modify the input file. This\n"
            "is handled via temporary file, such that the input file is not modified if any\n"
            "errors are encountered during processing.\n"));
  printf ("\n");

  printf (_("theoracomment handles comments in the format \"name=value\", one per line. By\n"
            "default, comments are written to stdout when listing, and read from stdin when\n"
            "editing. Alternatively, a file can be specified with the -c option, or tags\n"
            "can be given on the commandline with -t \"name=value\". Use of either -c or -t\n"
            "disables reading from stdin.\n"));
  printf ("\n");

  printf (_("Examples:\n"
            "  theoracomment -a in.ogg -c comments.txt\n"
            "  theoracomment -a in.ogg -t \"ARTIST=Some Guy\" -t \"TITLE=A Title\"\n"));
  printf ("\n");

  printf (_("NOTE: Raw mode (--raw, -R) will read and write comments in UTF-8 rather than\n"
            "converting to the user's character set, which is useful in scripts. However,\n"
            "this is not sufficient for general round-tripping of comments in all cases.\n"));
}

void free_param(param_t *param)
{
  int i;
  for(i=0; i!=param->commentcount; ++i)
    free(param->comments[i]);
  free(param->comments);
  free(param->infilename);
  free(param->outfilename);
  free(param);
}

/**********

   allocate and initialize a the parameter struct

***********/

param_t *new_param(void)
{
	param_t *param = (param_t *)malloc(sizeof(param_t));

	/* mode and flags */
	param->mode = MODE_LIST;
	param->raw = 0;

	/* filenames */
	param->infilename  = NULL;
	param->outfilename = NULL;
	param->commentfilename = "-";	/* default */

	/* file pointers */
	inputFile = outputFile = NULL;
	param->commentfile = NULL;
	param->tempoutfile=0;

	/* comments */
	param->commentcount=0;
	param->comments=NULL;

	return param;
}

/**********
   parse_options()

   This function takes care of parsing the command line options
   with getopt() and fills out the param struct with the mode,
   flags, and filenames.

***********/

void parse_options(int argc, char *argv[], param_t *param)
{
	int ret;
	int option_index = 1;

	setlocale(LC_ALL, "");

	while ((ret = getopt_long(argc, argv, "alwhqVc:t:R",
			long_options, &option_index)) != -1) {
		switch (ret) {
			case 0:
				fprintf(stderr, _("Internal error parsing command options\n"));
				exit(1);
				break;
			case 'l':
				param->mode = MODE_LIST;
				break;
			case 'R':
				param->raw = 1;
				break;
			case 'w':
				param->mode = MODE_WRITE;
				break;
			case 'a':
				param->mode = MODE_APPEND;
				break;
			case 'V':
				fprintf(stderr, "theoracomment from theora-tools " VERSION "\n");
				exit(0);
				break;
			case 'h':
				usage();
				exit(0);
				break;
			case 'q':
				/* set quiet flag: unused */
				break;
			case 'c':
				param->commentfilename = strdup(optarg);
				break;
			case 't':
				param->comments = realloc(param->comments, 
						(param->commentcount+1)*sizeof(char *));
				param->comments[param->commentcount++] = strdup(optarg);
				break;
			default:
				usage();
				exit(1);
		}
	}

	/* remaining bits must be the filenames */
	if((param->mode == MODE_LIST && (argc-optind) != 1) ||
	   ((param->mode == MODE_WRITE || param->mode == MODE_APPEND) &&
	   ((argc-optind) < 1 || (argc-optind) > 2))) {
			usage();
			exit(1);
	}

	param->infilename = strdup(argv[optind]);
	if (param->mode == MODE_WRITE || param->mode == MODE_APPEND)
	{
		if(argc-optind == 1)
		{
			param->tempoutfile = 1;
			param->outfilename = malloc(strlen(param->infilename)+8);
			strcpy(param->outfilename, param->infilename);
			strcat(param->outfilename, ".vctemp");
		}
		else
			param->outfilename = strdup(argv[optind+1]);
	}
}

/**********
   open_files()

   This function takes care of opening the appropriate files
   based on the mode and filenames in the param structure.
   A filename of '-' is interpreted as stdin/out.

   The idea is just to hide the tedious checking so main()
   is easier to follow as an example.

***********/

void open_files(param_t *p)
{
	/* for all modes, open the input file */

	if (strncmp(p->infilename,"-",2) == 0) {
    fprintf(stderr, _("Input from stdin is not supported at the moment.\n"));
    exit(EXIT_FAILURE);
	} else {
		inputFile = fopen(p->infilename, "rb");
	}
	if (!inputFile) {
		fprintf(stderr,
			_("Error opening input file '%s'.\n"),
			p->infilename);
		exit(1);
	}

	if (p->mode == MODE_WRITE || p->mode == MODE_APPEND) { 

		/* open output for write mode */
        if(!strcmp(p->infilename, p->outfilename)) {
            fprintf(stderr, _("Input filename may not be the same as output filename\n"));
            exit(1);
        }

		if (strncmp(p->outfilename,"-",2) == 0) {
			outputFile = stdout;
		} else {
			outputFile = fopen(p->outfilename, "wb");
		}
		if(!outputFile) {
			fprintf(stderr,
				_("Error opening output file '%s'.\n"),
				p->outfilename);
			exit(1);
		}

		/* commentfile is input */
		
		if ((p->commentfilename == NULL) ||
				(strncmp(p->commentfilename,"-",2) == 0)) {
			p->commentfile = stdin;
		} else {
			p->commentfile = fopen(p->commentfilename, "r");
		}
		if (!p->commentfile) {
			fprintf(stderr,
				_("Error opening comment file '%s'.\n"),
				p->commentfilename);
			exit(1);
		}

	} else {

		/* in list mode, commentfile is output */

		if ((!p->commentfilename) ||
				(strncmp(p->commentfilename,"-",2) == 0)) {
			p->commentfile = stdout;
		} else {
			p->commentfile = fopen(p->commentfilename, "w");
		}
		if (!p->commentfile) {
			fprintf(stderr,
				_("Error opening comment file '%s'\n"),
				p->commentfilename);
			exit(1);
		}
	}

	/* all done */
}

/**********
   close_files()

   Do some quick clean-up.

***********/

void close_files(param_t *p, int output_written)
{
  if (inputFile && inputFile != stdin) fclose(inputFile);
  if (outputFile && outputFile != stdout) fclose(outputFile);
  if (p->commentfile && p->commentfile != stdout && p->commentfile != stdin)
    fclose(p->commentfile);

  if(p->tempoutfile) {
#if HAVE_STAT && HAVE_CHMOD
    struct stat st;
    stat (p->infilename, &st);
#endif
    
    if(output_written) {
      /* Some platforms fail to rename a file if the new name already 
       * exists, so we need to remove, then rename. How stupid.
       */
      if(rename(p->outfilename, p->infilename)) {
        if(remove(p->infilename))
          fprintf(stderr, _("Error removing old file %s\n"), p->infilename);
        else if(rename(p->outfilename, p->infilename)) 
          fprintf(stderr, _("Error renaming %s to %s\n"), p->outfilename, 
                  p->infilename);
      } else {
#if HAVE_STAT && HAVE_CHMOD
        chmod (p->infilename, st.st_mode);
#endif
      }
    }
    else {
      if(remove(p->outfilename)) {
        fprintf(stderr, _("Error removing erroneous temporary file %s\n"), 
                    p->outfilename);
      }
    }
  }
}

