namespace ModPorter.Core;

// A deliberately small grammar for registration-only DllMain bodies. Unknown
// conditions, assignments, calls, preprocessor branches and thread notifications
// with work are rejected rather than silently removed from a port.
public sealed record RegistrationPlan(IReadOnlyList<string> Initialize, IReadOnlyList<string> Stop, ulong RequiredCapabilities) {
    public bool Extended => Initialize.Count != 0 || Stop.Count != 0;
}
public static class DllRegistration {
    public static RegistrationPlan? Parse(IReadOnlyList<Token> tokens, int arguments, int close, int end, string entry) {
        var parameters = new List<List<string>> { new() };
        for (int i=arguments+1;i<close;i++) {
            if(tokens[i].Text==",")parameters.Add(new());else parameters[^1].Add(tokens[i].Text);
        }
        if(parameters.Count!=3 || parameters.Any(p=>p.Count<2))return null;
        string module=parameters[0][^1],reason=parameters[1][^1];
        var p=new Parser(tokens,close+2,end,module,reason,entry);
        return p.Parse();
    }
    sealed class Parser(IReadOnlyList<Token> tokens,int start,int end,string module,string reason,string entry) {
        int at=start,depth;bool main;ulong capabilities;readonly List<string> init=new(),stop=new();
        string Current => at<end?tokens[at].Text:"";
        bool Take(string value){if(Current!=value)return false;++at;return true;}
        static bool Id(string text)=>text.Length>0&&(char.IsAsciiLetter(text[0])||text[0]=='_')&&text.All(c=>char.IsAsciiLetterOrDigit(c)||c=='_');
        static int Phase(string text)=>text switch {"DLL_PROCESS_ATTACH"=>1,"DLL_PROCESS_DETACH"=>2,"DLL_THREAD_ATTACH" or "DLL_THREAD_DETACH"=>3,_=>0};
        bool Statement(int phase) {
            if(Take(";"))return true;
            if(Take("{")){
                if(++depth>64)return false;
                while(Current!="}"&&at<end)if(!Statement(phase))return false;
                --depth;return Take("}");
            }
            string function=Current;if(!Id(function))return false;++at;
            if(!Take("("))return false;
            var args=new List<string>();
            if(Current!=")")for(;;){if(!Id(Current)&&!uint.TryParse(Current,out _))return false;args.Add(Current);++at;if(!Take(","))break;}
            if(!Take(")")||!Take(";"))return false;
            if(function=="DisableThreadLibraryCalls")return phase!=2&&args.SequenceEqual(new[]{module});
            if(phase is not 1 and not 2)return false;
            if(function is "scriptRegister" or "scriptRegisterAdditionalThread") {
                if(phase!=1||args.Count!=2||args[0]!=module||!Id(args[1]))return false;
                if(args[1]==entry){main=true;return true;}
                capabilities|=1;init.Add("if (!scriptRegisterAdditionalThread("+args[1]+")) return false;");return true;
            }
            if(function=="scriptUnregister") {
                if(phase!=2||args.Count!=1)return false;
                // The loader already cancels the entire owned group before Stop.
                return args[0]==module || Id(args[0]);
            }
            if(args.Count!=1||!Id(args[0]))return false;
            if(phase==1&&function is "presentCallbackRegister" or "renderCallbackRegister" or "keyboardHandlerRegister") {
                capabilities|=function=="keyboardHandlerRegister"?64u:32u;
                string checkedName=function=="renderCallbackRegister"?function:function+"Checked";
                init.Add("if (!"+checkedName+"("+args[0]+")) return false;");return true;
            }
            if(phase==2&&function is "presentCallbackUnregister" or "renderCallbackUnregister" or "keyboardHandlerUnregister") {
                stop.Add("// "+function+"("+args[0]+") is managed by the runtime, retaining Load Now support.");return true;
            }
            return false;
        }
        bool If() {
            do {
                if(!Take("if")||!Take("("))return false;
                string left=Current;++at;if(!Take("=="))return false;string right=Current;++at;
                int phase=left==reason?Phase(right):right==reason?Phase(left):0;
                if(phase==0||!Take(")")||!Statement(phase))return false;
            } while(Take("else"));
            return true;
        }
        bool Switch() {
            if(!Take("switch")||!Take("(")||!Take(reason)||!Take(")")||!Take("{"))return false;
            while(Current!="}"&&at<end) {
                int phase;
                if(Take("case")){phase=Phase(Current);++at;if(phase==0||!Take(":"))return false;}
                else if(Take("default")){phase=3;if(!Take(":"))return false;}
                else return false;
                bool braced=Take("{");
                while(Current!="break"&&Current!="}"&&at<end)if(!Statement(phase))return false;
                if(!Take("break")||!Take(";"))return false;
                if(braced&&!Take("}"))return false;
            }
            return Take("}");
        }
        public RegistrationPlan? Parse() {
            bool returned=false;
            while(at<end) {
                if(Current=="if"){if(!If())return null;}
                else if(Current=="switch"){if(!Switch())return null;}
                else if(Take("return")){if(Current is not "TRUE" and not "true" and not "1")return null;++at;if(!Take(";")||at!=end)return null;returned=true;}
                else if(!Statement(0))return null;
            }
            return returned&&main?new RegistrationPlan(init,stop,capabilities):null;
        }
    }
}
