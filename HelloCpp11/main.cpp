#include <functional>
#include <list>

void funA()
{
	printf("funA\n");
}

int main()
{
	std::function<void()> funa(funA);
	funa();

	auto f = []() {};

	return 0;
}