#ifdef __x86_64__
.globl blake3_cpuid
.globl blake3_xgetbv
.text

blake3_cpuid:
	push	%rbx
	mov	%esi, %eax
	mov	%edx, %ecx
	cpuid
	mov	%eax, 0(%rdi)
	mov	%ebx, 4(%rdi)
	mov	%ecx, 8(%rdi)
	mov	%edx, 12(%rdi)
	pop	%rbx
	ret

blake3_xgetbv:
	xor	%ecx, %ecx
	xgetbv
	ret

.section .ctors,"aw"
.align	8
.quad	blake3_detect_cpu_features
#endif
