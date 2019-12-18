#include "urlParser.h"
#include <stdio.h>
#include <string.h>

URL_parsed get_host_path(char *partOfUrl){
	URL_parsed ret;
	memset(ret.host, 	 0, 256);
	memset(ret.path, 	 0, 1024);
	memset(ret.filename, 0, 512);

	ret.success = 0;
	char *path;
	if (partOfUrl[0] == '/')
	{
		return ret; //empty host
	}

	path = strstr(partOfUrl, "/");
	if(path==NULL){
		//file doesn't seem to exist
		return ret;
	}
	int len=path - (partOfUrl);
	strncpy(ret.host, partOfUrl, len);
	strncpy(ret.path, path+1, 1024); // the +1 skips the slash

	if(strstr(ret.path, "/")==NULL){
		//file is in root
		strncpy(ret.filename, ret.path, 512);
		memset(ret.path, 0, 1024);
	}else{
		char* index=ret.path;
		char* oldIndex;
		while(1){
			oldIndex=index;
			index = strstr(index+1, "/");
			if (index==NULL){
				break;
			}
		}
		//last slash
		strcpy(ret.filename, oldIndex+1);
		memset(oldIndex+1, 0, oldIndex-ret.path);
	}
	ret.success = 1;
	return ret;
}

URL_parsed load_URL(char *url)
{
	URL_parsed ret;
	ret.success = 0;
	memset(ret.protocol, 0, 6);
	memset(ret.username, 0, 256);
	memset(ret.password, 0, 256);
	memset(ret.host, 	 0, 256);
	memset(ret.path, 	 0, 1024);
	memset(ret.filename, 0, 512);
	char *index;
	char *mk1;
	int len;

	index = strstr(url, "://");
	if (index == NULL)
	{
		return ret;
	}

	mk1 = index+3;	   // store marker1
	len = index - url; // protocol length
	if (len > 5 || len < 1)
	{
		return ret; //protocol length not valid
	}
	else
	{
		strncpy(ret.protocol, url, len);
	}

	//checking for '@'. It can only be one or none.
	index = url;
	int i = -1;
	do
	{
		index++; //1 char forward
		index = strstr(index, "@");
		i++;
	} while (index != NULL);

	if (i > 0){ // [<user>:<password>@] tag used
		char uap[512]; // username and password
		memset(uap, 0, 512); //clear memory
		index = strstr(mk1, "@");
		if(i>1){
			//'@' inside the uap tag
			index = strstr(index+1, "@"); //index of '@'
		}
		len = index - mk1;
		strncpy(uap, mk1, len);
		//<user>:<password> tag in uap variable
		index=strstr(uap, ":");
		if(index==NULL){
			//no password, so the whole tag will be treated as an username
			strcpy(ret.username, uap);
		}else{
			strncpy(ret.username, uap, index-uap);
			strcpy(ret.password, index+1); // skips ':'
		}
		//update mk1
		mk1+=strlen(uap)+1;
	}

	URL_parsed hap = get_host_path(mk1);
	if (hap.success == 0)
	{
		return ret;
	}
	else
	{
		strcpy(ret.host, hap.host);
		strcpy(ret.path, hap.path);
		strcpy(ret.filename, hap.filename);
	}
	if(strlen(ret.username)==0){
		strcpy(ret.username, "anonymous");
		strcpy(ret.password, "randompwd");
	}
	ret.success=1;
	return ret;
}
