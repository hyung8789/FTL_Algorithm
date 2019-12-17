#include "header.h"

FlashMem *flash = NULL;

extern int mapping_method = 2; //현재 플래시 메모리의 매핑방식 (default : 2 - 사용하지 않음)

void main() {
	flash->chk_initialized(&flash,mapping_method); //이미 생성된 플래시 메모리가 있는지 확인

	while (1)
	{
		flash->disp_command(mapping_method); //커맨드 출력
		flash->disp_flash_info(&flash, mapping_method); //플래시 메모리 정보 출력
		flash->input_command(&flash,mapping_method); //커맨드 입력
	}
}