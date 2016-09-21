#ifndef _IThread_h
#define _IThread_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

//���Ԍn�̓~���b����

//--------------------------------------------------------�}�N��--------------------------------------------------------

#define ULMAX 4294967295 //unsigned long�̍ő�l
#define SetTimerStop(Timer) Timer = ULMAX //�^�C�}�[����肤��ő�l�ɂ��邱�Ƃɂ���Ď����I�Ɏ~�߂�B�����\��������͖̂�50����̈�u

//--------------------------------------------------------�N���X�̒�`--------------------------------------------------------


/*
�[��������s���o����悤�ɂ���N���X�x�[�X
�g�p�̍ۂ̓��[�v����Tick�����s����B
*/
class ThreadBase
{
public:
	virtual bool Tick() = 0; //�����̎��s�B���[�v���Ŏg�p����B�^�C�}�[�ɂ���ď������N�����Ƃ���True��Ԃ��悤�ɂ���B
	virtual void Start() = 0; //�^�C�}�[�̊J�n�������B
	virtual void Stop() = 0; //�^�C�}�[�̒�~�������B
	virtual	void SetIntervalTimer(unsigned long Time) //IntervalTimer��C�ӂ̎��Ԃɐݒ肷��B����ɂ���ď����̃^�C�~���O�𒲐��o����B
	{
		IntervalTimer = Time;
	}
	virtual bool isRunning() //�^�C�}�[�����邱�Ƃœ����Ă��邩���m�F����B
	{
		return IntervalTimer != ULMAX; //SetTimerStop���^�C�}�[���ő�l�ɂ��邱�Ƃɂ���ă^�C�}�[���~�߂Ă��邽�߁B
	}

protected:
	unsigned long IntervalTimer;
};


/*
ThreadBase�ɂ��ȈՃ^�C�}�[�@�\�B
Tick�����[�v���Ŏg�p���邱�Ƃ�Interval���Ƃ�Tick��True��Ԃ��A�������삳���邱�Ƃ��ł���B
*/
class SimpleTimerThread : public ThreadBase
{
public:
	virtual bool Tick()
	{
		if ( millis()> IntervalTimer )
		{
			UpdateIntervalTImer();
			return true;
		}
		return false;
	}
	virtual void Start()
	{
		IntervalTimer = millis() + Interval;
	}
	virtual void Stop()
	{
		SetTimerStop(IntervalTimer);
	}
	virtual void SetInterval(unsigned long interval)
	{
		Interval = interval;
	}

protected:
	unsigned long Interval;
	void UpdateIntervalTImer() //�^�C�}�[���m���Ɍ��ݎ��Ԃ�薢���ɂȂ�悤�ɂ��邱�ƂŌ듮���h��
	{
		unsigned long NowTime = millis();
		while ( IntervalTimer < NowTime )
		{
			IntervalTimer += Interval;
		}
	}
};

#endif