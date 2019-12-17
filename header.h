#define _CRT_SECURE_NO_WARNINGS //_CRT_SECURE_NO_WARNINGS
#include <stdio.h> //fscanf,fprinf,fwrite,fread
#include <iostream> //�����
#include <sstream> //stringstream
#include <windows.h> //�ý��� ��ɾ�
#define MB_PER_BLOCK 64 //1MB�� 64 ���
#define MB_PER_SECTOR 2048 //1MB�� 2048����
#define MB_PER_BYTE 1048576 //1MB�� ����Ʈ ȯ�갪(MB_PER_SECTOR * BYTE_PER_SECTOR / MB_PER_BLOCK)
#define BLOCK_PER_SECTOR 32 //�� ��Ͽ� �ش��ϴ� ������ ����
#define BYTE_PER_BLOCK 16384 //�� ��Ͽ� �ش��ϴ� ����Ʈ�� ũ��(BYTE_PER_SECTOR * BLOCK_PER_SECTOR)
#define BYTE_PER_SECTOR 512 //�� ���Ϳ� �ش��ϴ� ����Ʈ�� ũ��
#define MAPPING_INIT_VALUE -1 //���� ���̺��� �ʱⰪ

extern int mapping_method; //���� �÷��� �޸��� ���ι�� (default : 2 - ������� ����)

// x64�� �������Ͽ��� size_t�� unsigned __int64�� �ν�
// �÷��� �޸��� �Ҵ�� ��ü �뷮�� ���� Free Space�� �����ϵ��� ���� �ʾ����Ƿ� �� �̻� ����� ������ ���� ��� �����߻� �� �� ����

/***
1���� = 512byte
1��� = 32���� = 32*512 = 16,384byte
1MB = 1024KB
1KB = 1024byte
1MB�� ���� �� = (1*1024*1024)/512(1���ʹ� byte) = 2048��
1MB�� ��� ��(32����) = 2048/32 = 64��
--
unsigned int arrange == unsigned long arrange : 0 ~ 4,294,967,295
4 Gigabyte�� byte ������ ��ȯ�ϸ� 4,294,967,296byte�̹Ƿ� �ڷ����� ������ �Ѿ
����, �� ū �뷮�� �÷��� �޸𸮸� �����ϱ� ���ؼ���
unsigned __int64�� ����Ͽ��� �ϴµ� ���� ������ _fseeki64�� __int64(long long)������ �����ϹǷ� __int64�� ���
--
unsigned __int64 arrange: 0 ~ 18,446,744,073,709,551,615
__int64 arrange : ?9,223,372,036,854,775,808 ~ 9,223,372,036,854,775,807
***/

class FlashMem {
public:
	FlashMem();
	FlashMem(__int64 megabytes); //megabytes ũ���� �÷��� �޸� ����

	__int64 *block_free_space; //��� ���� �������� ���� ���̺�
	__int64 *mapping_table; //���� ���̺�

	void disp_command(int mapping_method); //Ŀ�ǵ� ��ɾ� ���
	void input_command(FlashMem **flashmem, int &mapping_method); //Ŀ�ǵ� ��ɾ� �Է�

	void chk_initialized(FlashMem **flashmem, int &mapping_method); //�÷��� �޸𸮰� �̹� �����Ǿ� �ִ��� Ȯ��
	void disp_flash_info(FlashMem **flashmem, int mapping_method); //���� ������ �÷��� �޸��� ���� ���
	void load_table(FlashMem **flashmem, int mapping_method); //���� ��Ŀ� ���� ���� ���̺� �Ǵ� ��� ���� �������� ���̺� �ε�
	void save_table(FlashMem **flashmem, int mapping_method); //���� ��Ŀ� ���� ���� ���̺� �Ǵ� ��� ���� �������� ���̺� ����
	int switch_mapping_method(); //���� �÷��� �޸��� ���� ��� ����
	
	__int64 get_flashmem_size(); //�÷��� �޸� ũ�� ��ȯ
	__int64 calc_block_size(); //�Ҵ�� �޸� ũ�⿡ �ش��ϴ� ����� ���� ��ȯ
	__int64 calc_sector_size(); //�Ҵ�� �޸� ũ�⿡ �ش��ϴ� ������ ���� ��ȯ
	__int64 calc_storage_byte(); //�Ҵ�� �޸� ũ�⿡ �ش��ϴ� �� byte ũ�⸦ ��ȯ

private:
	__int64 flashmem_size; //�÷��� �޸��� MBũ��
};

int init(FlashMem **flashmem, __int64 megabytes, int mapping_method); //megabytes ũ���� �÷��� �޸𸮸� �Ҵ�
char Flash_read(FlashMem **flashmem, __int64 PSN, int mapping_method); //���� ���Ϳ� �����͸� �о��
int Flash_write(FlashMem **flashmem, __int64 PSN, char data, int mapping_method); //���� ���Ϳ� �����͸� ���
int Flash_erase(FlashMem **flashmem, __int64 PBN, int mapping_method); //���� ��Ͽ� �ش��ϴ� �����͸� ����

//FTL Algorithm
void Print_table(FlashMem **flashmem, int mapping_method);  //���� ���̺� ���
int FTL_read(FlashMem **flashmem, __int64 LSN, int mapping_method); //�� ���� �Ǵ� �� ��Ͽ� �ش�Ǵ� �������̺� �� ���� ���� �Ǵ� ���� ����� ��ġ�� ��ȯ
int FTL_write(FlashMem **flashmem, __int64 LSN, char data, int mapping_method); //�� ���� �Ǵ� �� ��Ͽ� �ش�Ǵ� �������̺� �� ���� ���� �Ǵ� ���� ��� ��ġ�� ���