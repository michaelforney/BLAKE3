#ifdef __x86_64__
.text
detect_cpu_features:
	push	%rbx

	mov	$1, %eax
	cpuid

	// SSE4.1
	mov	%ecx, %edi
	shr	$19, %edi
	and	$1, %edi

	mov	%edi, %esi

	// OSXSAVE
	test	$0x08000000, %ecx
	jz	.L1

	xor	%ecx, %ecx
	xgetbv

	// AVX usable
	mov	%esi, %ecx
	or	$2, %ecx
	mov	%eax, %edx
	and	$0x06, %edx
	cmp	$0x06, %edx
	cmove	%ecx, %esi

	// AVX512 usable
	mov	%esi, %ecx
	or	$4, %ecx
	and	$0xe6, %eax
	cmp	$0xe6, %eax
	cmove	%ecx, %esi

.L1:
	// extended features
	xor	%eax, %eax
	cpuid
	cmp	$7, %eax
	jb	.L2
	mov	$7, %eax
	xor	%ecx, %ecx
	cpuid

	// AVX2
	mov	%ebx, %ecx
	shr	$4, %ecx
	and	$2, %ecx
	or	%ecx, %edi

	// AVX512
	mov	%edi, %ecx
	or	$4, %ecx
	and	$0x80010000, %ebx
	cmp	$0x80010000, %ebx
	cmove	%ecx, %edi

.L2:
	and	%esi, %edi
	mov	%edi, blake3_cpu_features(%rip)

	pop	%rbx
	ret

.section .ctors,"a"
.align	8
.quad	detect_cpu_features

.bss
.align	4
.globl	blake3_cpu_features
blake3_cpu_features:
	.zero 4
#endif
