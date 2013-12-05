#include<stdint.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

typedef enum cbor_major_t
{
	#define cbor_major_t_min cbor_major_uint
	cbor_major_uint = 0 <<5,
	cbor_major_nint = 1 <<5,
	cbor_major_bstr = 2 <<5,
	cbor_major_tstr = 3 <<5,
	cbor_major_arr = 4 <<5,
	cbor_major_map = 5 <<5,
	cbor_major_tag = 6 <<5,
	cbor_major_flt = 7 <<5,
	#define cbor_major_t_max cbor_major_flt
	
	#define cbor_major_mask cbor_major_uint | cbor_major_nint | cbor_major_bstr | cbor_major_tstr | cbor_major_arr | cbor_major_map | cbor_major_tag | cbor_major_flt
} cbor_major_t;

struct cbor_t {
	const cbor_major_t major;
	struct cbor_t*next;
};

struct cbor_uint_t {
	struct cbor_t base;
	const uint64_t value;
};

uint64_t cbor_value_uint(const uint8_t additional,const int stream)
{
	uint64_t v=0;
	switch(additional) {
		default:
			return additional;
		case 24:
			(void)read(stream,&v,sizeof(uint8_t));
			break;
		case 25:
			(void)read(stream,&v,sizeof(uint16_t));
			break;
		case 26:
			(void)read(stream,&v,sizeof(uint32_t));
			break;
		case 27:
			(void)read(stream,&v,sizeof(uint64_t));
			break;
		case 28:
		case 30:
			// if v == 0 && additional != 0: error
			break;
	}
	return v;
}

int cbor_store_uint(struct cbor_t*storage,const uint8_t additional,const int stream)
{
	if(storage!=NULL && storage->next!=NULL)return 2;
	
	struct cbor_uint_t c= {
		.base= {
			.major=cbor_major_uint,
			.next=NULL,
		},
		.value=cbor_value_uint(additional,stream),
	},*fresh=malloc(sizeof*fresh);

	if(fresh==NULL)return 1;
	if(c.value==0&&additional!=0)return 3;
	
	memcpy(fresh,&c,sizeof*fresh);

	if(storage==NULL)
		storage=&fresh->base;
	else
		storage->next=&fresh->base;
	return EXIT_SUCCESS;
}

struct cbor_tag_t {
	struct cbor_t base;
	const uint64_t value;
};

int cbor_store_tag(struct cbor_t*storage,const uint8_t additional, const int stream)
{
	if(storage!=NULL && storage->next!=NULL)return 2;

	struct cbor_tag_t t= {
		.base= {
			.major=cbor_major_tag,
			.next=NULL,
		},
		.value=cbor_value_uint(additional,stream),
	},*fresh=malloc(sizeof*fresh);

	if(fresh==NULL)return 1;
	if(t.value==0&&additional!=0)return 3;

	memcpy(fresh,&t,sizeof*fresh);

	if(storage==NULL)
		storage=&fresh->base;
	else
		storage->next=&fresh->base;
	return EXIT_SUCCESS;
}

int(*const cbor_store[cbor_major_t_max])(struct cbor_t*,const uint8_t,const int stream) = {
	&cbor_store_uint,
	NULL,//&cbor_store_nint,
	NULL,//&cbor_store_bstr,
	NULL,//&cbor_store_tstr,
	NULL,//&cbor_store_arr,
	NULL,//&cbor_store_map,
	&cbor_store_tag,
	NULL,//&cbor_store_flt,
};

uint8_t cbor_major_of(uint8_t item)
{
	return (item& (cbor_major_mask))>>5;
}

uint8_t cbor_additional_of(uint8_t item)
{
	return item& ~(cbor_major_mask);
}

int decode(const int stream,struct cbor_t*storage)
{
	int wat=0;
	struct cbor_t*store=storage;
	do
	{
		uint8_t item;
	
		if(read(stream,&item,sizeof item) < 1 )
		{
			break;
			return EXIT_FAILURE;
		}

		wat=cbor_store[cbor_major_of(item)](store,cbor_additional_of(item),stream);
		if(store!=NULL)store=store->next;
	}
	while(wat==0);
	return EXIT_SUCCESS;
}