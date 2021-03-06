/*
 * Copyright © 2005 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Novell, Inc. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

//#define D(x) do { printf(__FILE__ ":%d:\t", __LINE__); printf x; fflush(stdout); } while(0)
#define D(x)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <dirent.h>

#include <ecomp.h>

CompPlugin *plugins = 0;

static Bool
dlloaderLoadPlugin(CompPlugin *p,
                   char       *path,
                   char       *name)
{
   char *file;
   void *dlhand;

   file = malloc ((path ? strlen (path) : 0) + strlen (name) + 8);
   if (!file)
     return FALSE;

   if (path)
     sprintf (file, "%s/lib%s.so", path, name);
   else
     sprintf (file, "lib%s.so", name);

   dlhand = dlopen (file, RTLD_LAZY);
   if (dlhand)
     {
        PluginGetInfoProc getInfo;
        char *error;

        dlerror ();

        getInfo = (PluginGetInfoProc)dlsym (dlhand, "getCompPluginInfo");

        error = dlerror ();
        if (error)
          {
             compLogMessage (NULL, "core", CompLogLevelError,
                             "dlsym: %s", error);

             getInfo = 0;
          }

        if (getInfo)
          {
             p->vTable = (*getInfo)();
             if (!p->vTable)
               {
                  compLogMessage (NULL, "core", CompLogLevelError,
                                  "Couldn't get vtable from '%s' plugin",
                                  file);

                  dlclose (dlhand);
                  free (file);

                  return FALSE;
               }
          }
        else
          {
             compLogMessage (NULL, "core", CompLogLevelError,
                             "Failed to lookup getCompPluginInfo in '%s' "
                             "plugin\n", file);

             dlclose (dlhand);
             free (file);

             return FALSE;
          }
     }
   else
     {
        free (file);
        D(("some other stuff went wrondg :(\n"));

        return FALSE;
     }

   free (file);

   p->devPrivate.ptr = dlhand;
   p->devType = "dlloader";

   return TRUE;
}

static void
dlloaderUnloadPlugin(CompPlugin *p)
{
   dlclose (p->devPrivate.ptr);
}

static int
dlloaderFilter(const struct dirent *name)
{
   int length = strlen (name->d_name);

   if (length < 7)
     return 0;

   if (strncmp (name->d_name, "lib", 3) ||
       strncmp (name->d_name + length - 3, ".so", 3))
     return 0;

   return 1;
}

static char **
dlloaderListPlugins(char *path,
                    int  *n)
{
   struct dirent **nameList;
   char **list;
   char *name;
   int length, nFile, i, j = 0;

   if (!path)
     path = ".";

   nFile = scandir (path, &nameList, dlloaderFilter, alphasort);
   if (!nFile)
     return 0;

   list = malloc (nFile * sizeof (char *));
   if (!list)
     return 0;

   for (i = 0; i < nFile; i++)
     {
        length = strlen (nameList[i]->d_name);

        name = malloc ((length - 5) * sizeof (char));
        if (name)
          {
             strncpy (name, nameList[i]->d_name + 3, length - 6);
             name[length - 6] = '\0';

             list[j++] = name;
          }
     }

   if (j)
     {
        *n = j;

        return list;
     }

   free (list);

   return NULL;
}

LoadPluginProc loaderLoadPlugin = dlloaderLoadPlugin;
UnloadPluginProc loaderUnloadPlugin = dlloaderUnloadPlugin;
ListPluginsProc loaderListPlugins = dlloaderListPlugins;

Bool
initPluginForDisplay(CompPlugin  *p,
                     CompDisplay *d)
{
   CompScreen *s, *failedScreen = d->screens;
   Bool status = TRUE;

   for (s = d->screens; s; s = s->next)
     {
        if (p->vTable->initScreen)
          {
             if (!(*p->vTable->initScreen)(p, s))
               {
                  failedScreen = s;
                  status = FALSE;
                  break;
               }
          }

        if (!(*s->initPluginForScreen)(p, s))
          {
             compLogMessage (NULL, "core", CompLogLevelError,
                             "Plugin '%s':initScreen failed",
                             p->vTable->name);

             if (p->vTable->finiScreen)
               (*p->vTable->finiScreen)(p, s);

             failedScreen = s;
             status = FALSE;
             break;
          }
     }

   for (s = d->screens; s != failedScreen; s = s->next)
     {
        (*s->finiPluginForScreen)(p, s);

        if (p->vTable->finiScreen)
          (*p->vTable->finiScreen)(p, s);
     }

   return status;
}

void
finiPluginForDisplay(CompPlugin  *p,
                     CompDisplay *d)
{
   CompScreen *s;

   for (s = d->screens; s; s = s->next)
     {
        (*s->finiPluginForScreen)(p, s);

        if (p->vTable->finiScreen)
          (*p->vTable->finiScreen)(p, s);
     }
}

Bool
initPluginForScreen(CompPlugin *p,
                    CompScreen *s)
{
   Bool status = TRUE;

   if (p->vTable->initWindow)
     {
        CompWindow *w, *failedWindow = s->windows;

        for (w = s->windows; w; w = w->next)
          {
             if (w->attrib.class != InputOnly)
               if (!(*p->vTable->initWindow)(p, w))
                 {
                    compLogMessage (NULL, "core", CompLogLevelError,
                                    "Plugin '%s':initWindow "
                                    "failed", p->vTable->name);
                    failedWindow = w;
                    status = FALSE;
                    break;
                 }
          }

        if (p->vTable->finiWindow)
          {
             for (w = s->windows; w != failedWindow; w = w->next)
               if (w->attrib.class != InputOnly)
                 (*p->vTable->finiWindow)(p, w);
          }
     }

   return status;
}

void
finiPluginForScreen(CompPlugin *p,
                    CompScreen *s)
{
   if (p->vTable->finiWindow)
     {
        CompWindow *w;

        for (w = s->windows; w; w = w->next)
          if (w->attrib.class != InputOnly)
            (*p->vTable->finiWindow)(p, w);
     }
}

static Bool
initPlugin(CompPlugin *p)
{
   CompDisplay *d = compDisplays;
   int version;

   version = (*p->vTable->getVersion)(p, ABIVERSION);
   if (version != ABIVERSION)
     {
        compLogMessage (NULL, "core", CompLogLevelError,
                        "Can't load plugin '%s' because it is built for "
                        "ABI version %d and actual version is %d",
                        p->vTable->name, version, ABIVERSION);
        return FALSE;
     }

   if (!(*p->vTable->init)(p))
     {
        compLogMessage (NULL, "core", CompLogLevelError,
                        "InitPlugin '%s' failed", p->vTable->name);
        return FALSE;
     }

   if (d)
     {
        if (p->vTable->initDisplay)
          {
             if (!(*p->vTable->initDisplay)(p, d))
               {
                  (*p->vTable->fini)(p);

                  return FALSE;
               }
          }

        if (!(*d->initPluginForDisplay)(p, d))
          {
             compLogMessage (NULL, "core", CompLogLevelError,
                             "Plugin '%s':initDisplay failed",
                             p->vTable->name);

             (*p->vTable->fini)(p);

             return FALSE;
          }
     }

   return TRUE;
}

static void
finiPlugin(CompPlugin *p)
{
   CompDisplay *d = compDisplays;

   if (d)
     {
        (*d->finiPluginForDisplay)(p, d);

        if (p->vTable->finiDisplay)
          (*p->vTable->finiDisplay)(p, d);
     }

   (*p->vTable->fini)(p);
}

void
screenInitPlugins(CompScreen *s)
{
   CompPlugin *p;
   int i, j = 0;

   for (p = plugins; p; p = p->next)
     j++;

   while (j--)
     {
        i = 0;
        for (p = plugins; i < j; p = p->next)
          i++;

        if (p->vTable->initScreen)
          (*s->initPluginForScreen)(p, s);
     }
}

void
screenFiniPlugins(CompScreen *s)
{
   CompPlugin *p;

   for (p = plugins; p; p = p->next)
     {
        if (p->vTable->finiScreen)
          (*s->finiPluginForScreen)(p, s);
     }
}

void
windowInitPlugins(CompWindow *w)
{
   CompPlugin *p;

   if (w->attrib.class == InputOnly) return;

   for (p = plugins; p; p = p->next)
     {
        if (p->vTable->initWindow)
          (*p->vTable->initWindow)(p, w);
     }
}

void
windowFiniPlugins(CompWindow *w)
{
   CompPlugin *p;

   if (w->attrib.class == InputOnly) return;

   for (p = plugins; p; p = p->next)
     {
        if (p->vTable->finiWindow)
          (*p->vTable->finiWindow)(p, w);
     }
}

CompPlugin *
findActivePlugin(char *name)
{
   CompPlugin *p;

   for (p = plugins; p; p = p->next)
     {
        if (strcmp (p->vTable->name, name) == 0)
          return p;
     }

   return 0;
}

void
unloadPlugin(CompPlugin *p)
{
   (*loaderUnloadPlugin)(p);
   free (p);
}

CompPlugin *
loadPlugin(char *name)
{
   CompPlugin *p;
   char *home, *plugindir;
   Bool status;

   p = malloc (sizeof (CompPlugin));
   if (!p)
     return 0;

   p->next = 0;
   p->devPrivate.uval = 0;
   p->devType = NULL;
   p->vTable = 0;

   home = getenv ("HOME");
   if (home)
     {
        plugindir = malloc (strlen (home) + strlen (HOME_PLUGINDIR) + 3);
        if (plugindir)
          {
             sprintf (plugindir, "%s/%s", home, HOME_PLUGINDIR);
             status = (*loaderLoadPlugin)(p, plugindir, name);
             free (plugindir);

             if (status)
               return p;
          }
     }

   status = (*loaderLoadPlugin)(p, PLUGINDIR, name);
   if (status)
     return p;

   status = (*loaderLoadPlugin)(p, NULL, name);
   if (status)
     return p;

   compLogMessage (NULL, "core", CompLogLevelError, "Couldn't load plugin '%s'", name);

   return 0;
}

Bool
pushPlugin(CompPlugin *p)
{
   if (findActivePlugin (p->vTable->name))
     {
        compLogMessage (NULL, "core", CompLogLevelWarn,
                        "Plugin '%s' already active",
                        p->vTable->name);

        return FALSE;
     }

   p->next = plugins;
   plugins = p;

   if (!initPlugin (p))
     {
        compLogMessage (NULL, "core", CompLogLevelError,
                        "Couldn't activate plugin '%s'", p->vTable->name);
        plugins = p->next;

        return FALSE;
     }

   return TRUE;
}

CompPlugin *
popPlugin(void)
{
   CompPlugin *p = plugins;

   if (!p)
     return 0;

   finiPlugin (p);

   plugins = p->next;

   return p;
}

CompPlugin *
getPlugins(void)
{
   return plugins;
}

static Bool
stringExist(char **list,
            int    nList,
            char  *s)
{
   int i;

   for (i = 0; i < nList; i++)
     if (strcmp (list[i], s) == 0)
       return TRUE;

   return FALSE;
}

char **
availablePlugins(int *n)
{
   char *home, *plugindir;
   char **list, **currentList, **pluginList, **homeList = NULL;
   int nCurrentList, nPluginList, nHomeList;
   int count, i, j;

   home = getenv ("HOME");
   if (home)
     {
        plugindir = malloc (strlen (home) + strlen (HOME_PLUGINDIR) + 3);
        if (plugindir)
          {
             sprintf (plugindir, "%s/%s", home, HOME_PLUGINDIR);
             homeList = (*loaderListPlugins)(plugindir, &nHomeList);
             free (plugindir);
          }
     }

   pluginList = (*loaderListPlugins)(PLUGINDIR, &nPluginList);
   currentList = (*loaderListPlugins)(".", &nCurrentList);

   count = 0;
   if (homeList)
     count += nHomeList;
   if (pluginList)
     count += nPluginList;
   if (currentList)
     count += nCurrentList;

   if (!count)
     return NULL;

   list = malloc (count * sizeof (char *));
   if (!list)
     return NULL;

   j = 0;
   if (homeList)
     {
        for (i = 0; i < nHomeList; i++)
          if (!stringExist (list, j, homeList[i]))
            list[j++] = homeList[i];

        free (homeList);
     }

   if (pluginList)
     {
        for (i = 0; i < nPluginList; i++)
          if (!stringExist (list, j, pluginList[i]))
            list[j++] = pluginList[i];

        free (pluginList);
     }

   if (currentList)
     {
        for (i = 0; i < nCurrentList; i++)
          if (!stringExist (list, j, currentList[i]))
            list[j++] = currentList[i];

        free (currentList);
     }

   *n = j;

   return list;
}

