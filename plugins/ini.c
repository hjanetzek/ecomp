/* * Copyright © 2009 Hannes Janetzek
 *
 * Copyright © 2007 Mike Dransfield
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Mike
 * Dransfield not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Mike Dransfield makes no representations about the
 * suitability of this software for any purpose. It is provided "as
 * is" without express or implied warranty.
 *
 * MIKE DRANSFIELD AND HANNES JANETZEK DISCLAIM ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL MIKE DRANSFIELD BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 *
 * based on ini plugin by: Mike Dransfield <mike@blueroot.co.uk>
 *
 * Some code taken from gconf.c by : David Reveman <davidr@novell.com>
 */

#define _GNU_SOURCE /* for asprintf */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <ecomp.h>
#include <Eina.h>
#include <Eet.h>

#define MAX_OPTION_LENGTH	1024
#define HOME_OPTIONDIR	   ".ecomp"
#define CORE_NAME			"core"

#define GET_INI_DISPLAY(d)									\
	((IniDisplay *) (d)->privates[displayPrivateIndex].ptr)
#define INI_DISPLAY(d)							\
	IniDisplay *id = GET_INI_DISPLAY (d)
#define GET_INI_SCREEN(s, id)									\
	((IniScreen *) (s)->privates[(id)->screenPrivateIndex].ptr)
#define INI_SCREEN(s)													\
	IniScreen *is = GET_INI_SCREEN (s, GET_INI_DISPLAY (s->display))

#define NUM_OPTIONS(s) (sizeof ((s)->opt) / sizeof (CompOption))

static int displayPrivateIndex;

static CompMetadata iniMetadata;

static Bool iniSaveOptions (CompDisplay *d,
							int			screen,
							char		*plugin);

static Eet_Data_Descriptor *edd_group, *edd_option;


typedef struct _Option
{
	int type;

	int intValue;
	double doubleValue;
	char *stringValue;
	Eina_List *listValue;
}Option;

typedef struct _Group
{
	Eina_Hash *data;
}Group;



static Eina_Hash*
eet_eina_hash_add(Eina_Hash *hash, const char *key, const void *data)
{
	if (!hash) hash = eina_hash_string_superfast_new(NULL);
	if (!hash) return NULL;

	eina_hash_add(hash, key, data);
	return hash;
}


/*
 * IniFileData
 */
typedef struct _IniFileData IniFileData;
struct _IniFileData {
  char		 *filename;
  char		 *plugin;
  int			 screen;

  Bool		 blockWrites;
  Bool		 blockReads;

  IniFileData		 *next;
  IniFileData		 *prev;
};

/*
 * IniDisplay
 */
typedef struct _IniDisplay {
	int						  screenPrivateIndex;

	CompFileWatchHandle		  directoryWatch;
	int locked;

	InitPluginForDisplayProc	  initPluginForDisplay;
	SetDisplayOptionProc	  setDisplayOption;
	SetDisplayOptionForPluginProc setDisplayOptionForPlugin;

	IniFileData			*fileData;
} IniDisplay;

/*
 * IniScreeen
 */
typedef struct _IniScreen {
	InitPluginForScreenProc		   initPluginForScreen;
	SetScreenOptionProc		   setScreenOption;
	SetScreenOptionForPluginProc   setScreenOptionForPlugin;
} IniScreen;


/* static IniFileData *
 * iniGetFileDataFromFilename (CompDisplay *d,
 * 			    const char *filename)
 * {
 *   int len, i;
 *   int pluginSep = 0, screenSep = 0;
 *   char *pluginStr, *screenStr;
 *   IniFileData *fd;
 * 
 *   INI_DISPLAY (d);
 * 
 *   if (!filename)
 *     return NULL;
 * 
 *   len = strlen (filename);
 * 
 *   if (len < (strlen(FILE_SUFFIX) + 2))
 *     return NULL;
 * 
 *   if ((filename[0]=='.') || (filename[len-1]=='~'))
 *     return NULL;
 * 
 *   for (fd = id->fileData; fd; fd = fd->next)
 *     if (strcmp (fd->filename, filename) == 0)
 *       return fd;
 * 
 *   for (i=0; i<len; i++)
 *     {
 *       if (filename[i] == '-')
 * 	{
 * 	  if (!pluginSep)
 * 	    pluginSep = i-1;
 * 	  else
 * 	    return NULL; /\*found a second dash *\/
 * 	}
 *       else if (filename[i] == '.')
 * 	{
 * 	  if (!screenSep)
 * 	    screenSep = i-1;
 * 	  else
 * 	    return NULL; /\*found a second dot *\/
 * 	}
 *     }
 * 
 *   if (!pluginSep || !screenSep)
 *     return NULL;
 * 
 *   /\* If we get here then there is no fd in the display variable *\/
 *   IniFileData *newFd = malloc (sizeof (IniFileData));
 *   if (!newFd)
 *     return NULL;
 * 
 *   /\* fd is NULL here, see condition "fd" in first for-loop *\/
 *   /\* if (fd)
 *      fd->next = newFd;
 *      else
 *   *\/
 *   id->fileData = newFd;
 * 
 *   newFd->prev = fd;
 *   newFd->next = NULL;
 * 
 *   newFd->filename = strdup (filename);
 * 
 *   pluginStr = calloc (1, sizeof (char) * pluginSep + 2);
 *   if (!pluginStr)
 *     return NULL;
 * 
 *   screenStr = calloc (1, sizeof (char) * (screenSep - pluginSep));
 *   if (!screenStr) {
 *     free(pluginStr);
 *     return NULL;
 *   }
 * 
 *   strncpy (pluginStr, filename, pluginSep + 1);
 *   strncpy (screenStr, &filename[pluginSep+2], (screenSep - pluginSep) - 1);
 * 
 *   if (strcmp (pluginStr, CORE_NAME) == 0)
 *     newFd->plugin = NULL;
 *   else
 *     newFd->plugin = strdup (pluginStr);
 * 
 *   if (strcmp (screenStr, "allscreens") == 0)
 *     newFd->screen = -1;
 *   else
 *     newFd->screen = atoi(&screenStr[6]);
 * 
 *   newFd->blockReads  = FALSE;
 *   newFd->blockWrites = FALSE;
 * 
 *   free (pluginStr);
 *   free (screenStr);
 * 
 *   return newFd;
 * } */

static char *
iniOptionValueToString (CompOptionValue *value, CompOptionType type)
{
	char tmp[MAX_OPTION_LENGTH];
	tmp[0] = '\0';

	switch (type)
	{
	/* case CompOptionTypeBool:
	 * case CompOptionTypeInt:
	 * 	snprintf(tmp, 256, "%i", (int)value->i);
	 * 	break;
	 * case CompOptionTypeFloat:
	 * 	snprintf(tmp, 256, "%f", value->f);
	 * 	break; */
	case CompOptionTypeString:
		snprintf (tmp, MAX_OPTION_LENGTH, "%s", strdup (value->s));
		break;
	case CompOptionTypeColor:
		snprintf (tmp, 10, "%s", colorToString (value->c));
		break;
	case CompOptionTypeMatch:
	{
		char *s = matchToString (&value->match);
		snprintf (tmp, MAX_OPTION_LENGTH, "%s", s);
		free(s);
	}
	break;
	default:
		break;
	}

	return strdup (tmp);
}

static Bool
iniGetHomeDir (char **homeDir)
{
	char *home = NULL, *tmp;

	home = getenv ("HOME");
	if (home)
	{
		tmp = malloc (strlen (home) + strlen (HOME_OPTIONDIR) + 2);
		if (tmp)
		{
			sprintf (tmp, "%s/%s", home, HOME_OPTIONDIR);
			(*homeDir) = strdup (tmp);
			free (tmp);

			return TRUE;
		}
	}

	return FALSE;
}

static Bool
iniGetFilename (CompDisplay *d,
				int screen,
				char *plugin,
				char **filename)
{
	CompScreen *s;
	int		   len;
	char	   *fn = NULL, *screenStr;

	screenStr = malloc (sizeof(char) * 12);
	if (!screenStr)
		return FALSE;

	if (screen > -1)
	{
		for (s = d->screens; s ; s = s->next)
			if (s && (s->screenNum == screen))
				break;

		if (!s)
		{
			compLogMessage (d, "ini", CompLogLevelWarn,
							"Invalid screen number passed " \
							"to iniGetFilename %d", screen);
			free(screenStr);
			return FALSE;
		}
		snprintf (screenStr, 12, "screen%d", screen);
	}
	else
	{
		strncpy (screenStr, "allscreens", 12);
	}

	len = strlen (screenStr) + 2;

	if (plugin)
		len += strlen (plugin);
	else
		len += strlen (CORE_NAME);

	fn = malloc (sizeof (char) * len);
	if (fn)
	{
		sprintf (fn, "%s-%s",
				 plugin ? plugin : CORE_NAME, screenStr);

		*filename = strdup (fn);

		free (screenStr);
		free (fn);

		return TRUE;
	}

	free (screenStr);

	return FALSE;
}



static Bool
iniMakeDirectories (void)
{
	char *homeDir;

	if (iniGetHomeDir (&homeDir))
	{
		mkdir (homeDir, 0700);
		free (homeDir);

		return TRUE;
	}
	else
	{
		compLogMessage (NULL, "ini", CompLogLevelWarn,
						"Could not get HOME environmental variable");
		return FALSE;
	}
}

#define OPT_SCREEN		   0
#define OPT_DISPLAY		   1
#define OPT_PLUGIN_SCREEN  2
#define OPT_PLUGIN_DISPLAY 3

typedef struct _IniOptData
{
	int type;
	CompScreen *s;
	CompDisplay *d;
	CompOption *option;
	int nOption;
	char *plugin;

}IniOptData;

static Bool
iniGetOptList(Option *listOpt, CompListValue *list, CompOptionType type)
{
	Eina_List *l;
	Option *opt;
	int count = eina_list_count(listOpt->listValue);
	int i = 0;

	if (count == 0)
		return FALSE;

	list->nValue = count;
	list->value = malloc (sizeof (CompOptionValue) * count);
	printf(" - >");
	for (l = listOpt->listValue; l; l = l->next, i++)
	{
		opt = l->data;
		switch (type)
		{
		case CompOptionTypeString:
			list->value[i].s = strdup (opt->stringValue);
			printf("%s ", opt->stringValue);
			break;
		case CompOptionTypeBool:
			list->value[i].b = (Bool) opt->intValue;
			printf("%d ", opt->intValue);
			break;
		case CompOptionTypeInt:
			list->value[i].i = opt->intValue;
			printf("%d ", opt->intValue);
			break;
		case CompOptionTypeFloat:
			list->value[i].f = (float) opt->doubleValue;
			printf("%f ", opt->doubleValue);
			break;
		case CompOptionTypeMatch:
			matchInit (&list->value[i].match);
			matchAddFromString (&list->value[i].match, opt->stringValue);
			printf("%s ", opt->stringValue);
			break;
		default:
			break;
		}
	}
	printf("\n");
	return TRUE;
}

static Eina_Bool
iniFreeGroup(const Eina_Hash *hash, const void *key, void *data, void *fdata)
{
	Option *opt = data;

	if (opt->stringValue) free (opt->stringValue);
	if (opt->listValue)
	{
		Option *item;
		EINA_LIST_FREE(opt->listValue, item)
		{
			if (item->stringValue) free (item->stringValue);
			free(item);
		}
	}

	free(opt);

	return 1;
}


static Eina_Bool
iniLoadGroup(const Eina_Hash *hash, const void *key, void *data, void *fdata)
{
	IniOptData *od = (IniOptData *) fdata;
	Option *opt = data;
	char *optionName = (char *) key, *optionValue;

	CompOption *option = od->option;
	int nOption = od->nOption;
	CompScreen *s = od->s;
	CompDisplay *d = od->d;
	char *plugin = od->plugin;

	CompOption *o;
	CompOptionValue value;
	int hasValue = FALSE, status;

	optionValue = opt->stringValue;

	o = compFindOption (option, nOption, optionName, 0);
	if (o)
	{
		value = o->value;

		switch (o->type)
		{
		case CompOptionTypeBool:
			printf("load: %s: %d\n", optionName, opt->intValue);
			hasValue = TRUE;
			value.b = (Bool) opt->intValue;
			break;
		case CompOptionTypeInt:
			printf("load: %s: %d\n", optionName, opt->intValue);
			hasValue = TRUE;
			value.i = opt->intValue;
			break;
		case CompOptionTypeFloat:
			printf("load: %s: %f\n", optionName, opt->doubleValue);
			hasValue = TRUE;
			value.f = (float) opt->doubleValue;
			break;
		case CompOptionTypeString:
			printf("load: %s: %s\n", optionName, opt->stringValue);
			hasValue = TRUE;
			value.s = strdup (opt->stringValue);
			break;
		case CompOptionTypeColor:
			printf("load: %s: %s\n", optionName, opt->stringValue);
			hasValue = stringToColor (optionValue, value.c);
			break;
		case CompOptionTypeList:
			printf("load list: %s\n", optionName);
			hasValue = iniGetOptList(opt, &value.list, value.list.type);
			break;
		case CompOptionTypeMatch:
			printf("load: %s: %s\n", optionName, opt->stringValue);
			hasValue = TRUE;
			matchInit (&value.match);
			matchAddFromString (&value.match, optionValue);
			break;
		default:
			break;
		}

		if (hasValue)
		{
			switch(od->type)
			{
			case OPT_PLUGIN_SCREEN:
				status = (*s->setScreenOptionForPlugin) (s, plugin, optionName, &value);
				break;
			case OPT_PLUGIN_DISPLAY:
				status = (*d->setDisplayOptionForPlugin) (d, plugin, optionName, &value);
				break;
			case OPT_SCREEN:
				status = (*s->setScreenOption) (s, optionName, &value);
				break;
			case OPT_DISPLAY:
				status = (*d->setDisplayOption) (d, optionName, &value);
				break;
			default:
				break;
			}

			if (o->type == CompOptionTypeMatch)
			{
				matchFini (&value.match);
			}
		}
	}

	return 1;
}

static Bool
iniLoadOptionsFromFile (CompDisplay *d, Group *options, char *plugin, int screen)
{
	CompOption		*option = NULL;
	CompScreen		*s = NULL;
	CompPlugin		*p = NULL;
	int				nOption; //, nOptionRead = 0;
	IniOptData od;

	if (plugin)
	{
		p = findActivePlugin (plugin);
		if (!p)
		{
			compLogMessage (d, "ini", CompLogLevelWarn,
							"Could not find running plugin " \
							"%s (iniLoadOptionsFromFile)", plugin);
			return FALSE;
		}
	}

	if (screen > -1)
	{
		for (s = d->screens; s; s = s->next)
			if (s && s->screenNum == screen)
				break;

		if (!s)
		{
			compLogMessage (d, "ini", CompLogLevelWarn,
							"Invalid screen number passed to " \
							"iniLoadOptionsFromFile %d", screen);
			return FALSE;
		}
	}

	if (plugin && p)
	{
		if (s && p->vTable->getScreenOptions)
		{
			option = (*p->vTable->getScreenOptions) (p, s, &nOption);
			od.type = OPT_PLUGIN_SCREEN;
		}
		else if (p->vTable->getDisplayOptions)
		{
			option = (*p->vTable->getDisplayOptions) (p, d, &nOption);
			od.type = OPT_PLUGIN_DISPLAY;
		}

		od.plugin = plugin;
	}
	else
	{
		if (s)
		{
			option = compGetScreenOptions (s, &nOption);
			od.type = OPT_SCREEN;
		}
		else
		{
			option = compGetDisplayOptions (d, &nOption);
			od.type = OPT_DISPLAY;
		}
	}

	od.d = d;
	od.s = s;
	od.option = option;
	od.nOption = nOption;

	eina_hash_foreach(options->data, iniLoadGroup, &od);
	eina_hash_foreach(options->data, iniFreeGroup, NULL);
	free (options);
	
	return TRUE;
}

static Bool
iniSaveOptions (CompDisplay *d,
				int			screen,
				char		*plugin)
{
	INI_DISPLAY(d);
	if (id->locked) return TRUE;

	CompScreen *s = NULL;
	CompOption *option;
	int		   nOption = 0;
	char	   *filename, *directory, *fullPath, *strVal = NULL;

	if (screen > -1)
	{
		for (s = d->screens; s; s = s->next)
			if (s && s->screenNum == screen)
				break;

		if (!s)
		{
			compLogMessage (d, "ini", CompLogLevelWarn,
							"Invalid screen number passed to " \
							"iniSaveOptions %d", screen);
			return FALSE;
		}
	}

	if (plugin)
	{
		CompPlugin *p;
		p = findActivePlugin (plugin);
		if (!p)
			return FALSE;

		if (s)
			option = (*p->vTable->getScreenOptions) (p, s, &nOption);
		else
			option = (*p->vTable->getDisplayOptions) (p, d, &nOption);
	}
	else
	{
		/* core (general) setting */
		if (s)
			option = compGetScreenOptions (s, &nOption);
		else
			option = compGetDisplayOptions (d, &nOption);
	}

	if (!option)
		return FALSE;

	if (!iniGetFilename (d, screen, plugin, &filename))
		return FALSE;

	if (!iniGetHomeDir (&directory))
		return FALSE;

	fullPath = malloc (sizeof (char) * (strlen (directory) + 10));
	sprintf (fullPath, "%s/%s", directory, "ecomp.eet");
	printf("open write %s\n", filename);

	Eet_File *optionFile = eet_open(fullPath, EET_FILE_MODE_READ_WRITE);
	Group *options = eet_data_read(optionFile, edd_group, filename);
	Option *opt;
	if (!options)
	{
		printf("calloc options\n");
		options = calloc (1, sizeof (Group));
	}

	if (!optionFile)
	{
		compLogMessage (d, "ini", CompLogLevelError,
						"Failed to write to %s, check you " \
						"have the correct permissions", fullPath);
		free (filename);
		free (directory);
		free (fullPath);
		return FALSE;
	}

	Bool status;

	while (nOption--)
	{
		status = FALSE;
		int i;

		switch (option->type)
		{
		case CompOptionTypeBool:
			if (!(opt = eina_hash_find(options->data, option->name)))
			{
				opt = calloc(1, sizeof(Option));
				opt->type = option->type;
				options->data = eet_eina_hash_add(options->data, option->name, opt);
			}
			opt->intValue = (int) option->value.b;
			break;
		case CompOptionTypeInt:
			if (!(opt = eina_hash_find(options->data, option->name)))
			{
				opt = calloc(1, sizeof(Option));
				opt->type = option->type;
				options->data = eet_eina_hash_add(options->data, option->name, opt);
			}
			opt->intValue = option->value.i;
			break;
		case CompOptionTypeFloat:
			if (!(opt = eina_hash_find(options->data, option->name)))
			{
				opt = calloc(1, sizeof(Option));
				opt->type = option->type;
				options->data = eet_eina_hash_add(options->data, option->name, opt);
			}
			opt->doubleValue = (double) option->value.f;
			break;			
		case CompOptionTypeString:
		case CompOptionTypeColor:
		case CompOptionTypeMatch:
			strVal = iniOptionValueToString (&option->value, option->type);
			if (strVal)
			{
				if (!(opt = eina_hash_find(options->data, option->name)))
				{
					opt = calloc(1, sizeof(Option));
					opt->type = option->type;
					options->data = eet_eina_hash_add(options->data, option->name, opt);
				}
				else if (opt->stringValue)
					free (opt->stringValue);

				opt->stringValue = strVal;

				printf("add option %s: %s\n", option->name, opt->stringValue);
			}
			break;
		case CompOptionTypeList:
			switch (option->value.list.type)
			{
			case CompOptionTypeBool:
			case CompOptionTypeInt:
			case CompOptionTypeFloat:
			case CompOptionTypeString:
			case CompOptionTypeColor:
			case CompOptionTypeMatch:
			{
				char *itemVal;

				if (!(opt = eina_hash_find(options->data, option->name)))
				{
					opt = calloc(1, sizeof(Option));
					opt->type = CompOptionTypeList;
					options->data = eet_eina_hash_add(options->data, option->name, opt);
				}

				printf("add option %s:\n", option->name);

				Option *item;
				CompListValue *list = &option->value.list;
				
				EINA_LIST_FREE(opt->listValue, item)
				{
					if (item->stringValue) free (item->stringValue);
					free(item);
				}
					
				for (i = 0; i < list->nValue; i++)
				{
					Option *o;

					switch (list->type)
					{
					case CompOptionTypeString:
					case CompOptionTypeMatch:
						itemVal = iniOptionValueToString (&list->value[i],
														  list->type);					
						if (itemVal)
						{
							o = calloc(1, sizeof(Option));
							o->type = list->type;
							o->stringValue = itemVal;
							opt->listValue = eina_list_append(opt->listValue, o);
						}
						break;
					case CompOptionTypeBool:
						o = calloc(1, sizeof(Option));
						o->type = list->type;
						o->intValue = (int) list->value[i].b;
						opt->listValue = eina_list_append(opt->listValue, o);
						break;
					case CompOptionTypeInt:
						o = calloc(1, sizeof(Option));
						o->type = list->type;
						o->intValue = list->value[i].i;
						opt->listValue = eina_list_append(opt->listValue, o);
						break;
					case CompOptionTypeFloat:
						o = calloc(1, sizeof(Option));
						o->type = list->type;
						o->doubleValue = (double) list->value[i].f;
						opt->listValue = eina_list_append(opt->listValue, o);
						break;			
					default:
						break;
					}
				}
				
				break;
			}
			default:
				compLogMessage (d, "ini", CompLogLevelWarn,
								"Unknown list option type %d, %s\n",
								option->value.list.type,
								optionTypeToString (option->value.list.type));
				break;
			}
			break;
		default:
			break;
		}

		option++;
	}

	if (!eet_data_write(optionFile, edd_group, filename, options, 1))
		fprintf(stderr, "Error writing data!\n");

	eet_close(optionFile);

	eina_hash_foreach(options->data, iniFreeGroup, NULL);

	free (options);	
	free (fullPath);
	free (filename);
	free (directory);
	
	return TRUE;
}

/* taken from ecore_file */
static int
copyFile(const char *src, const char *dst)
{
	FILE               *f1, *f2;
	char                buf[16384];
	char                realpath1[4096];
	char                realpath2[4096];
	size_t              num;
	int                 ret = 1;

	if (!realpath(src, realpath1)) return 0;
	if (realpath(dst, realpath2) && !strcmp(realpath1, realpath2)) return 0;

	f1 = fopen(src, "rb");
	if (!f1) return 0;
	f2 = fopen(dst, "wb");
	if (!f2)
	{
		fclose(f1);
		return 0;
	}
	while ((num = fread(buf, 1, sizeof(buf), f1)) > 0)
	{
		if (fwrite(buf, 1, num, f2) != num) ret = 0;
	}
	fclose(f1);
	fclose(f2);
	return ret;
}


static Bool
iniLoadOptions (CompDisplay *d, int screen, char *plugin)
{
	char *filename, *directory, *fullPath;
	Group *options;

	INI_DISPLAY (d);

	printf("iniLoadOptions\n");

	filename = directory = NULL;

	if (!iniGetFilename (d, screen, plugin, &filename))
		return FALSE;

	if (!iniGetHomeDir (&directory))
	{
		free (filename);
		return FALSE;
	}

	fullPath = malloc (sizeof (char) * (strlen (directory) + 11));
	sprintf (fullPath, "%s/%s", directory, "ecomp.eet");

	Eet_File *optionFile = eet_open(fullPath, EET_FILE_MODE_READ);

	if (!optionFile && iniMakeDirectories ())
	{
		char *tmpPath = malloc (sizeof (char) * (strlen (METADATADIR) + 11));
		sprintf (tmpPath, "%s/%s", METADATADIR, "ecomp.eet");
		printf("load defaults: %s\n", tmpPath);
		
		if (copyFile(tmpPath, fullPath))
			optionFile = eet_open(fullPath, EET_FILE_MODE_READ);
	}
	
	if (!optionFile) goto error;

	printf("open read %s\n", filename);

	optionFile = eet_open(fullPath, EET_FILE_MODE_READ);


	if (!optionFile) goto error;

	options = eet_data_read(optionFile, edd_group, filename);

	if (!options)
	{
		eet_close(optionFile);
		goto error;
	}

	id->locked = TRUE;
	iniLoadOptionsFromFile (d, options, plugin, screen);
	id->locked = FALSE;

	eet_close(optionFile);

	free (filename);
	free (directory);
	free (fullPath);
	return TRUE;

error:
	free (filename);
	free (directory);
	free (fullPath);
	return FALSE;
}

static void
iniFileModified (const char *name,
		 void *closure)
{
  CompDisplay *d;


  d = (CompDisplay *) closure;


  char *filename, *directory, *fullPath;
  Group *options;

  INI_DISPLAY (d);

  printf("iniFileModified\n");
  if(id->locked) return;
  
  filename = directory = NULL;

  if (!iniGetHomeDir (&directory))
    {
      free (filename);
      return;
    }

  fullPath = malloc (sizeof (char) * (strlen (directory) + 11));
  sprintf (fullPath, "%s/%s", directory, "ecomp.eet");

  Eet_File *optionFile = eet_open(fullPath, EET_FILE_MODE_READ);

  if (!optionFile) goto error;

  printf("open read %s\n", fullPath);


  optionFile = eet_open(fullPath, EET_FILE_MODE_READ);


  if (!optionFile) goto error;

  char **list;
  int num, i;
  
  list = eet_list(optionFile, "*", &num);
  if (!list)
    {
      eet_close(optionFile);
      goto error;
    }

  /* XXX we have only one for now */
  int screen = 0;;  
  char plugin[32];
  
  for (i = 0; i < num; i++)
    {
      options = eet_data_read(optionFile, edd_group, list[i]);

      if (!options)
	{
	  eet_close(optionFile);
	  goto error;
	}

      if(strstr(list[i], "screen0"))
	screen = 0;
      else
	screen = -1;
    
      char *end = list[i];
      int len = 1;
	  
      while (*end++ != '-')
	len ++;
	  
      snprintf(plugin, len, "%s", list[i]);
      printf("\nplug: %s - %d\n", plugin, screen);
    
      id->locked = TRUE;
      if(!strcmp("core", plugin))
	iniLoadOptionsFromFile (d, options, NULL, screen);
      else
	iniLoadOptionsFromFile (d, options, plugin, screen);
      id->locked = FALSE;
    }
   
  eet_close(optionFile);

 error:
  free (filename);
  free (directory);
  free (fullPath);
}


/*
  CORE FUNCTIONS
*/

static Bool
iniInitPluginForDisplay (CompPlugin	 *p,
						 CompDisplay *d)
{
	Bool status;

	INI_DISPLAY (d);

	UNWRAP (id, d, initPluginForDisplay);
	status = (*d->initPluginForDisplay) (p, d);
	WRAP (id, d, initPluginForDisplay, iniInitPluginForDisplay);

	if (status && p->vTable->getDisplayOptions)
	{
		iniLoadOptions (d, -1, p->vTable->name);
	}
	else if (!status)
	{
		compLogMessage (d, "ini", CompLogLevelWarn,
						"Plugin '%s' failed to initialize " \
						"display settings", p->vTable->name);
	}

	return status;
}

static Bool
iniInitPluginForScreen (CompPlugin *p,
						CompScreen *s)
{
	Bool status;

	INI_SCREEN (s);

	UNWRAP (is, s, initPluginForScreen);
	status = (*s->initPluginForScreen) (p, s);
	WRAP (is, s, initPluginForScreen, iniInitPluginForScreen);

	if (status && p->vTable->getScreenOptions)
	{
		iniLoadOptions (s->display, s->screenNum, p->vTable->name);
	}
	else if (!status)
	{
		compLogMessage (s->display, "ini", CompLogLevelWarn,
						"Plugin '%s' failed to initialize " \
						"screen %d settings", p->vTable->name, s->screenNum);
	}

	return status;
}

static Bool
iniSetScreenOption (CompScreen *s, char *name, CompOptionValue *value)
{
	Bool status;

	INI_SCREEN (s);

	UNWRAP (is, s, setScreenOption);
	status = (*s->setScreenOption) (s, name, value);
	WRAP (is, s, setScreenOption, iniSetScreenOption);

	if (status)
	{
		iniSaveOptions (s->display, s->screenNum, NULL);
	}

	return status;
}

static Bool
iniSetDisplayOption (CompDisplay *d, char *name, CompOptionValue *value)
{
	Bool status;

	INI_DISPLAY (d);

	UNWRAP (id, d, setDisplayOption);
	status = (*d->setDisplayOption) (d, name, value);
	WRAP (id, d, setDisplayOption, iniSetDisplayOption);

	if (status)
	{
		iniSaveOptions (d, -1, NULL);
	}

	return status;
}

static Bool
iniSetDisplayOptionForPlugin (CompDisplay	  *d,
							  char		  *plugin,
							  char		  *name,
							  CompOptionValue *value)
{
	Bool status;

	INI_DISPLAY (d);

	UNWRAP (id, d, setDisplayOptionForPlugin);
	status = (*d->setDisplayOptionForPlugin) (d, plugin, name, value);
	WRAP (id, d, setDisplayOptionForPlugin, iniSetDisplayOptionForPlugin);

	if (status)
	{
		CompPlugin *p;

		p = findActivePlugin (plugin);
		if (p && p->vTable->getDisplayOptions)
			iniSaveOptions (d, -1, plugin);
	}

	return status;
}

static Bool
iniSetScreenOptionForPlugin (CompScreen		 *s,
							 char		 *plugin,
							 char		 *name,
							 CompOptionValue *value)
{
	Bool status;

	INI_SCREEN (s);

	UNWRAP (is, s, setScreenOptionForPlugin);
	status = (*s->setScreenOptionForPlugin) (s, plugin, name, value);
	WRAP (is, s, setScreenOptionForPlugin, iniSetScreenOptionForPlugin);

	if (status)
	{
		CompPlugin *p;

		p = findActivePlugin (plugin);
		if (p && p->vTable->getScreenOptions)
			iniSaveOptions (s->display, s->screenNum, plugin);
	}

	return status;
}

static Bool
iniInitDisplay (CompPlugin *p, CompDisplay *d)
{
	IniDisplay *id;
	char *homeDir;

	id = malloc (sizeof (IniDisplay));
	if (!id)
		return FALSE;

	id->screenPrivateIndex = allocateScreenPrivateIndex (d);
	if (id->screenPrivateIndex < 0)
	{
		free (id);
		return FALSE;
	}

	id->directoryWatch = 0;

	id->locked = FALSE;

	WRAP (id, d, initPluginForDisplay, iniInitPluginForDisplay);
	WRAP (id, d, setDisplayOption, iniSetDisplayOption);
	WRAP (id, d, setDisplayOptionForPlugin, iniSetDisplayOptionForPlugin);

	d->privates[displayPrivateIndex].ptr = id;

	iniLoadOptions (d, -1, NULL);

	if (iniGetHomeDir (&homeDir))
	{
	id->directoryWatch = addFileWatch (d, homeDir,
					   NOTIFY_DELETE_MASK |
					   NOTIFY_CREATE_MASK |
					   NOTIFY_MODIFY_MASK,
					   iniFileModified, (void *) d);
	free (homeDir);
	}

	return TRUE;
}

static void
iniFiniDisplay (CompPlugin *p, CompDisplay *d)
{
	INI_DISPLAY (d);

	if (id->directoryWatch)
	removeFileWatch (d, id->directoryWatch);

	freeScreenPrivateIndex (d, id->screenPrivateIndex);

	UNWRAP (id, d, initPluginForDisplay);
	UNWRAP (id, d, setDisplayOption);
	UNWRAP (id, d, setDisplayOptionForPlugin);

	free (id);
}

static Bool
iniInitScreen (CompPlugin *p, CompScreen *s)
{
	IniScreen *is;

	INI_DISPLAY (s->display);

	is = malloc (sizeof (IniScreen));
	if (!is)
		return FALSE;

	s->privates[id->screenPrivateIndex].ptr = is;

	WRAP (is, s, initPluginForScreen, iniInitPluginForScreen);
	WRAP (is, s, setScreenOption, iniSetScreenOption);
	WRAP (is, s, setScreenOptionForPlugin, iniSetScreenOptionForPlugin);

	iniLoadOptions (s->display, s->screenNum, NULL);

	return TRUE;
}

static void
iniFiniScreen (CompPlugin *p, CompScreen *s)
{
	INI_SCREEN (s);

	UNWRAP (is, s, initPluginForScreen);
	UNWRAP (is, s, setScreenOption);
	UNWRAP (is, s, setScreenOptionForPlugin);

	free (is);
}

static Bool
iniInit (CompPlugin *p)
{
	if (!compInitPluginMetadataFromInfo (&iniMetadata, p->vTable->name,
										 0, 0, 0, 0))
		return FALSE;

	displayPrivateIndex = allocateDisplayPrivateIndex ();
	if (displayPrivateIndex < 0)
	{
		compFiniMetadata (&iniMetadata);
		return FALSE;
	}

	eina_init();
	eet_init();

	/* Option option;
	 * Group  group; */

	edd_group = eet_data_descriptor_new("group", sizeof(Group),
										NULL, NULL, NULL, NULL,
										(void  (*) (void *, int (*) (void *, const char *, void *, void *), void *))eina_hash_foreach,
										(void *(*) (void *, const char *, void *))eet_eina_hash_add,
										(void  (*) (void *))eina_hash_free);

	edd_option = eet_data_descriptor_new("option", sizeof(Option),
										 (void *(*) (void *))eina_list_next,
										 (void *(*) (void *, void *)) eina_list_append,
										 (void *(*) (void *))eina_list_data_get,
										 (void *(*) (void *))eina_list_free,
										 NULL, NULL, NULL);

	EET_DATA_DESCRIPTOR_ADD_BASIC(edd_option, Option, "type",	 type,		  EET_T_INT);
	EET_DATA_DESCRIPTOR_ADD_BASIC(edd_option, Option, "int",	 intValue,	  EET_T_INT);
	EET_DATA_DESCRIPTOR_ADD_BASIC(edd_option, Option, "double",	 doubleValue, EET_T_DOUBLE);
	EET_DATA_DESCRIPTOR_ADD_BASIC(edd_option, Option, "string",	 stringValue, EET_T_STRING);
	EET_DATA_DESCRIPTOR_ADD_LIST (edd_option, Option, "list",	 listValue,	  edd_option);

	EET_DATA_DESCRIPTOR_ADD_HASH (edd_group,  Group,  "options", data, edd_option);

	compAddMetadataFromFile (&iniMetadata, p->vTable->name);

	return TRUE;
}

static void
iniFini (CompPlugin *p)
{
	if (displayPrivateIndex >= 0)
		freeDisplayPrivateIndex (displayPrivateIndex);
	eet_shutdown();
	eina_shutdown();

}

static int
iniGetVersion (CompPlugin *plugin, int	version)
{
	return ABIVERSION;
}

static CompMetadata *
iniGetMetadata (CompPlugin *plugin)
{
	return &iniMetadata;
}

CompPluginVTable iniVTable = {
	"ini",
	iniGetVersion,
	iniGetMetadata,
	iniInit,
	iniFini,
	iniInitDisplay,
	iniFiniDisplay,
	iniInitScreen,
	iniFiniScreen,
	0,
	0,
	0,
	0
};

CompPluginVTable *
getCompPluginInfo (void)
{
	return &iniVTable;
}
