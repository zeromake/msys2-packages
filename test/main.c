#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


typedef struct CSPLIT_FRAGMENT
{
  char *text;
  struct CSPLIT_FRAGMENT *next;
  struct CSPLIT_FRAGMENT *prev;
} CSplitFragment_t;

typedef struct CSPLIT_LIST
{
  int num_elems;
  CSplitFragment_t *head;
  CSplitFragment_t *tail;
} CSplitList_t;

static int csplit_push_to_list(CSplitList_t *list, CSplitFragment_t *fragment, size_t buff_size)
{
  if (list == NULL || fragment == NULL)
  {
    return -1;
  }
  else
  {
    list->num_elems = list->num_elems + 1;
    if (list->head == NULL)
    {
      list->head = fragment;
      list->tail = fragment;
    }
    else
    {
      list->tail->next = fragment;
      fragment->prev = list->tail;
      list->tail = fragment;
    }
    fragment->text = (char *)calloc(1, buff_size);
  }
  return 0;
}

static int csplit_str(CSplitList_t *list, const char *input_str, const char *token) {
  int err = 0;
  int in_len = strlen(input_str);
  int token_len = strlen(token);
  const char *current_location = input_str;
  char *next_location;
  while (current_location != NULL)
  {
    CSplitFragment_t *fragment = (CSplitFragment_t *)calloc(1, sizeof(CSplitFragment_t));
    next_location = strstr(current_location, token);
    size_t req_buff_size;
    if (next_location == NULL)
      req_buff_size = in_len - (current_location - input_str);
    else if (next_location != NULL)
      req_buff_size = next_location - current_location;
    err = csplit_push_to_list(list, fragment, req_buff_size+1);
    if (err != 0) {
      return err;
    }
    strncpy(fragment->text, current_location, req_buff_size);
    if (next_location != NULL)
      current_location = next_location + token_len;
    else {
      current_location = next_location;
    }
  }
  return err;
}
static void freeList(CSplitList_t* list) {
  if (list != NULL) {
    return;
  }
  CSplitFragment_t* head = list->head;
  CSplitFragment_t* temp;
  while (head != NULL) {
    if (head->text != NULL) {
      free(head->text);
    }
    temp = head;
    head = head->next;
    if (temp != NULL) {
      free(temp);
    }
  }
  free(list);
}

static char* rel_path(const char *src, const char *dst) {
  CSplitList_t* srcSplit = (CSplitList_t*)calloc(1, sizeof(CSplitList_t));
  int ret = csplit_str(srcSplit, src, "/");
  if (ret != 0) {
    return NULL;
  }
  CSplitList_t* dstSplit = (CSplitList_t*)calloc(1, sizeof(CSplitList_t));
  ret = csplit_str(dstSplit, dst, "/");
  if (ret != 0) {
    return NULL;
  }
  CSplitFragment_t* srcHead = srcSplit->head;
  CSplitFragment_t* dstHead = dstSplit->head;
  while (srcHead != NULL && dstHead != NULL) {
    if (strcmp(srcHead->text, dstHead->text) != 0) {
      break;
    }
    srcHead = srcHead->next;
    dstHead = dstHead->next;
  }
  CSplitList_t* list = (CSplitList_t *)calloc(1, sizeof(CSplitList_t));
  size_t result_count = 0;
  while (dstHead->next != NULL){
    CSplitFragment_t *fragment = (CSplitFragment_t *)calloc(1, sizeof(CSplitFragment_t));
    csplit_push_to_list(list, fragment, 3);
    fragment->text = "..\0";
    dstHead = dstHead->next;
    result_count += 3;
  }
  while (srcHead != NULL)
  {
    CSplitFragment_t *fragment = (CSplitFragment_t *)calloc(1, sizeof(CSplitFragment_t));
    size_t count = strlen(srcHead->text);
    csplit_push_to_list(list, fragment, count+1);
    strncpy(fragment->text, srcHead->text, count);
    srcHead = srcHead->next;
    result_count += count;
    if (srcHead != NULL) {
      result_count += 1;
    }
  }
  freeList(srcSplit);
  freeList(dstSplit);
  char* result = (char*)calloc(result_count, sizeof(char));
  char* t = result;
  CSplitFragment_t* head = list->head;
  while (head != NULL)
  {
    size_t c = strlen(head->text);
    strncpy(t, head->text, c);
    t+=c;
    head = head->next;
    if (head != NULL) {
      t[0] = '/';
      t+=1;
    }
  }
  freeList(list);
  return result;
}

_Bool __stdcall CreateSymbolicLinkW(wchar_t *lpSymlinkFileName, wchar_t *lpTargetFileName, unsigned long dwFlags);

static wchar_t *char_to_wchar(const char *s, size_t count)
{
  size_t len = count + 1;
  wchar_t *WStr = (wchar_t *)malloc(len * sizeof(wchar_t));
  mbstowcs(WStr, s, len);
  return WStr;
}

static int win_create_symlink(const char *src, const char *dst, int is_dir)
{
  wchar_t *w_src = char_to_wchar(src, strlen(src));
  wchar_t *w_dst = char_to_wchar(dst, strlen(dst));
  _Bool ret = CreateSymbolicLinkW(w_dst, w_src, is_dir == 1 ? 1 : 0);
  free(w_src);
  free(w_dst);
  return ret ? 0 : 1;
}

static int symlinkat()


static int win_symlinkat(const char *dst, int fd, const char *src)
{
  int ret = 0;
  if (fd != -100)
  {
    ret = fchdir(fd);
    if (ret != 0)
    {
      return ret;
    }
  }
  char* rel = rel_path(src, dst);
  int i = strlen(dst);
  int count = i;
  for (; i >= 0; i--) {
    char c = dst[i];
    if (c == '/') {
      break;
    }
  }
  char* dir = (char*)calloc(i+1, sizeof(char));
  char* name = (char*)calloc(count - i, sizeof(char));
  strncpy(dir, dst, i);
  strncpy(name, dst+i+1, count - i-1);
  ret = chdir(dir);
  if (ret != 0)
  {
    return ret;
  }
  count = strlen(rel);
  for (i = 0;i < count; i++) {
    if (rel[i] == '/') {
      rel[i] = '\\';
    }
  }
  ret = win_create_symlink(rel, name, 0);
  free(dir);
  free(name);
  free(rel);
  return ret;
}

int main() {
  // char *src = "lua-5.4.3-1-x86_64.pkg.tar.zst";
  // char *dst = "1";
  // int s = win_create_symlink(dst, src, 0);
  // printf("1111:%d\n", s);
  int ret = win_symlinkat("usr/bin/git-receive-pack.exe", -100, "usr/lib/git-core/git.exe");
  // char* ret = rel_path("usr/lib/git-core/git.exe", "usr/bin/git-receive-pack.exe");
  printf("%d\n", ret);
}