#include "HVM.h"
#include "..\Utils\Utils.h"
#include "..\Intel\VMX.h"

BOOLEAN HvmIsHVSupported()
{
	CPU_VENDOR vendor = UtilCPUVendor();	//��ȡ�ĵ�CPU��Ϣ
	if (vendor == CPU_Intel)
	{
		return VmxHardSupported();
	}
	return TRUE;
}

/// <summary>
/// CPU virtualization features
/// </summary>
VOID HvmCheckFeatures()
{
	CPU_VENDOR vendor = UtilCPUVendor();
	if (vendor == CPU_Intel)
	{
		VmxCheckFeatures();
	}
		
}

/// <summary>
/// Virtualize each CPU
/// </summary>
/// <returns>Status code</returns>
NTSTATUS StartHV()
{
	// Unknown CPU
	if (g_Data->CPUVendor == CPU_Other)
		return STATUS_NOT_SUPPORTED;

	KeGenericCallDpc(HvmpHVCallbackDPC, (PVOID)__readcr3());		//���ö�ʱ��  �ڶ��������� Ӧ���Ǵ�����

																	// Some CPU failed
	ULONG count = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);
	//����ָ������ʧ�ܵ�����
	if (count != (ULONG)g_Data->vcpus)
	{
		//  DPRINT( "HyperBone: CPU %d: %s: Some CPU failed to subvert\n", CPU_IDX, __FUNCTION__ );
		StopHV();	//ȫ��ʧ��  ��ر�HV
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}
VOID HvmpHVCallbackDPC(PRKDPC Dpc, PVOID Context, PVOID SystemArgument1, PVOID SystemArgument2)
{
	UNREFERENCED_PARAMETER(Dpc);
	PVCPU pVCPU = &g_Data->cpu_data[CPU_IDX];

	// Check if we are loading, or unloading
	if (ARGUMENT_PRESENT(Context))	//ARGUMENT_PRESENT() NULL����FALSE 
	{
		// Initialize the virtual processor
		g_Data->CPUVendor == CPU_Intel ? IntelSubvertCPU(pVCPU, Context) : AMDSubvertCPU(pVCPU, Context);
	}
	else
	{
		// Tear down the virtual processor	���
		g_Data->CPUVendor == CPU_Intel ? IntelRestoreCPU(pVCPU) : AMDRestoreCPU(pVCPU);
	}

	// Wait for all DPCs to synchronize at this point
	KeSignalCallDpcSynchronize(SystemArgument2);

	// Mark the DPC as being complete
	KeSignalCallDpcDone(SystemArgument1);
}

//
// Vendor-specific calls
//
VOID IntelSubvertCPU(IN PVCPU Vcpu, IN PVOID SystemDirectoryTableBase)
{
	VmxInitializeCPU(Vcpu, (ULONG64)SystemDirectoryTableBase);
}
/// <summary>
/// Devirtualize each CPU
/// </summary>
/// <returns>Status code</returns>
NTSTATUS StopHV()
{
	// Unknown CPU
	if (g_Data->CPUVendor == CPU_Other)
		return STATUS_NOT_SUPPORTED;

	KeGenericCallDpc(HvmpHVCallbackDPC, NULL);

	return STATUS_SUCCESS;
}

VOID AMDSubvertCPU(IN PVCPU Vcpu, IN PVOID arg)
{
	UNREFERENCED_PARAMETER(Vcpu);
	UNREFERENCED_PARAMETER(arg);
	//DPRINT( "HyperBone: CPU %d: %s: AMD-V not yet supported\n", CPU_IDX, __FUNCTION__ );
} 
VOID IntelRestoreCPU(IN PVCPU Vcpu)
{
	// Prevent execution of VMCALL on non-vmx CPU
	if (Vcpu->VmxState > VMX_STATE_OFF)
		VmxShutdown(Vcpu);
}
VOID AMDRestoreCPU(IN PVCPU Vcpu)
{
	UNREFERENCED_PARAMETER(Vcpu);
	// DPRINT( "HyperBone: CPU %d: %s: AMD-V not yet supported\n", CPU_IDX, __FUNCTION__ );
}