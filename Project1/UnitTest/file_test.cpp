#include "stdafx.h"
#include "CppUnitTest.h"
#include "../Project1/file.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest
{
	TEST_CLASS(File_UnitTest)
	{
	public:

		TEST_METHOD(File_Create)
		{
			File file;
			int res = file.create("", 0);
			Assert::AreEqual(res, 0);
		}

	};
}