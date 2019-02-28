using System;
using System.Text.RegularExpressions;


namespace replace
{
    class MainClass
    {
        public static int Main(string[] args)
        {
            if (args.Length < 3)
            {
                Console.WriteLine("Usage: replace.exe pattern string file");
                return 1;
            }

            bool test = (args.Length == 4);
            int result = 0;

            try
            {
                string pattern = args[0];
                string str = args[1];
                string filepath = args[2];

                string text = System.IO.File.ReadAllText(filepath);
                string text_replaced = Regex.Replace(text, pattern, str);

                if (test)
                {
                    Console.WriteLine(text_replaced);
                }
                else
                {
                    System.IO.File.WriteAllText(filepath, text_replaced);
                }

            } catch (System.Exception e)
            {
                Console.WriteLine(e.Message);
                result = 1;
            }

            return result;
        }
    }
}
