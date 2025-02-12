#include <exception>

#include "app.hpp"

int main()
{

#ifdef NDEBUG
	std::cout << "Running NOT in debug mode" << '\n';
#else
	std::cout << "Running in debug mode" << '\n';
#endif

	Application app;

	try
	{
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
