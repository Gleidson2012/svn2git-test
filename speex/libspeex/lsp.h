/*---------------------------------------------------------------------------*\

	FILE........: AK2LSPD.H
	TYPE........: Turbo C header file
	COMPANY.....: Voicetronix
	AUTHOR......: James Whitehall
	DATE CREATED: 21/11/95

\*---------------------------------------------------------------------------*/

#ifndef __AK2LSPD__
#define __AK2LSPD__

int lpc_to_lsp (float *a, int lpcrdr, float *freq, int nb, float delta);
void lsp_to_lpc(float *freq, float *ak, int lpcrdr);

#endif	/* __AK2LSPD__ */
