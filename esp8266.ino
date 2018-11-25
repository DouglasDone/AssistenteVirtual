//WIFI Manager
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#define BOTtoken "560372982:AAFezd9cdEKtP7iO0vabN340CNlP2R79Tbw"  // Token do @configurarbot

#define PIN_D0 16
#define PIN_D1 5
#define PIN_D2 4
#define PIN_D3 0
#define PIN_D4 2
#define PIN_D5 14
#define PIN_D6 12
#define PIN_D7 13
#define PIN_D8 15
#define PIN_A0 17

#define BOT_SCAN_MESSAGE_INTERVAL 1000 //Intervalo para obter novas mensagens
long lastTimeScan;  // Ultima vez que buscou mensagem

String clientMac = "";

class Pino
{
  public:
    String nome = "";
    String nome_digital = "";  
};

class PinoConfiguracao
{
  public:
    String nome = "";
    String nome_digital = "";  
    String mac_dispositivo = "";
};

class Usuario
{
  public:
    String nome = "";
    String chat_id = ""; 
    int etapa = 0; 
};

class Dispositivo
{
      public:
        String mac = "";
        String nome = "";
        String token_id = "";
        String senha_mestra = "";
        String senha_acesso = "";
        int etapa_configuracao = 0;  
        Pino pinos[10]; 
        Usuario usuarios[10]; 
};

//Verifica se o mac já foi configurado
Dispositivo dispositivo;

void reiniciarDispositivo(){
  dispositivo.mac = "";
  dispositivo.nome = "";
  dispositivo.token_id = "";
  dispositivo.senha_mestra = "";
  dispositivo.senha_acesso = "";
  dispositivo.etapa_configuracao = 0;

  int c = 0;  
  while(c < sizeof(dispositivo.pinos)/sizeof(Pino)) {    
    dispositivo.pinos[c].nome = "";
    dispositivo.pinos[c].nome_digital = "";    
    c++;
  }

  c = 0;  
  while(c < sizeof(dispositivo.usuarios)/sizeof(Usuario)) {    
    dispositivo.usuarios[c].nome = "";
    dispositivo.usuarios[c].chat_id = "";    
    dispositivo.usuarios[c].etapa = 0; 
    c++;
  } 
  
}

PinoConfiguracao pino_configuracao;

WiFiClientSecure client;
UniversalTelegramBot bot_configuracao(BOTtoken, client);

bool formatoTokenValido(String valor) { 
  if (valor.length() == 45) {
    return true;
  }
  return false;
}


/*
ETAPAS DE CONFIGURAÇÃO
0 - Iniciando a configuração
1 - Aguardando 'sim' ou 'não' da configuração
2 - Aguardando Token Id
3 - Aguardando senha mestre
4 - Término da configuração inicial
5 - Conseguiu conversar com o Bot
*/
// Trata as mensagens que chegam ao Bot de Configuração
void mensagensBotConfiguracao(int numNewMessages) {

  int i=0;
    boolean entendeu = false;
    
    String chat_id = String(bot_configuracao.messages[i].chat_id);
    String text_chat = bot_configuracao.messages[i].text;
    String text = text_chat;

    // Pessoa que está enviando a mensagem
    String nome_usuario = bot_configuracao.messages[i].from_name;    
    if (nome_usuario == "") nome_usuario = "humano";

    // Tratamento para cada tipo de comando a seguir.
    text.toLowerCase();
    text = utf8ToLatin(text);

      if (text == "reiniciar") {
          bot_configuracao.sendMessage(chat_id, "Estou reiniciando a sua rede, só podemos conversar depois que você me reconectar.", "Markdown");
          WiFiManager wifiManager;
          
          if(!wifiManager.startConfigPortal("ESP8266") )
          {            
            ESP.restart();
          }    
      } 
      else if (dispositivo.etapa_configuracao == 0) 
        {
          String keyboardJson = "[[\"Sim\", \"Não\"]]";
          bot_configuracao.sendMessageWithReplyKeyboard(chat_id, "Olá " + nome_usuario + " eu sou o Bot de configuração e vi que você ainda não configurou o seu bot. Vamos começar?", "", keyboardJson, true);
          dispositivo.etapa_configuracao = 1;
          dispositivo.mac = clientMac;
          
        }
        else if (dispositivo.etapa_configuracao == 1)
        {
          if (text == "sim" || text == "/sim") {
             bot_configuracao.sendMessage(chat_id, "Preciso que você informe o Token Id do seu bot (essa informação é exibida após a criação do bot), caso não tenha basta enviar '/token' ao BotFather)", "Markdown");
             dispositivo.etapa_configuracao = 2;
          } else if (text == "nao" || text == "/nao") {
            bot_configuracao.sendMessage(chat_id, "Sem problemas. Quando precisar estou a disposição.", "Markdown");
            dispositivo.etapa_configuracao = 0;
          } else {
            String keyboardJson = "[[\Sim\", \"Não\"]]";
            bot_configuracao.sendMessageWithReplyKeyboard(chat_id, "Desculpe eu não entendi, vamos tentar outra vez. Eu vi que você ainda não configurou o seu bot, podemos começar?", "", keyboardJson, true);
            dispositivo.etapa_configuracao = 1;
          }        
        }
        else if (dispositivo.etapa_configuracao == 2) {
          if (formatoTokenValido(text)) {
            //Recupera o texto original (com o case sensitive)Armazena o Token
            dispositivo.token_id = text_chat;
            dispositivo.etapa_configuracao = 3;
            bot_configuracao.sendMessage(chat_id, "Feito. Preciso apenas que você me informe uma senha mestra de 4 a 8 caracteres.\nEssa senha será importante caso você queira reiniciar todo esse processo. Então me diga, qual será a sua senha?", "Markdown"); 
          } else {
            bot_configuracao.sendMessage(chat_id, "O token informado é inválido. Por favor, informe o Token correto.", "Markdown");            
          }
        }
        else if (dispositivo.etapa_configuracao == 3) {
          if (text.length() >= 4 && text.length() <=8) {
            bot_configuracao.sendMessage(chat_id, "Boa escolha. Já salvei a sua senha (pode ficar tranquilo que não conto pra ninguém).\nComigo é só, agora você já pode conversar diretamente com o seu Bot, aliás, ele está te esperando ansiosamente.", "Markdown");   
            dispositivo.senha_mestra = text;
            dispositivo.etapa_configuracao = 4;
          } else {
            bot_configuracao.sendMessage(chat_id, "Ops, essa senha não deu certo.\n A sua senha mestra deve ter entre 4 e 8 caracteres", "Markdown"); 
          }
        }           
}

String recuperarOpcoes(String chat_id) {
  String keyboardJsonOpcoes = "[";

  bool primeiro = true;
  
  int c = 0;
  while(c < sizeof(dispositivo.pinos)/sizeof(Pino)) {
    if (dispositivo.pinos[c].nome != "") {
      if (primeiro) {
        primeiro = false;
      } else {
        keyboardJsonOpcoes += ", ";
      }
      if (dispositivo.pinos[c].nome_digital == "a0") {
        keyboardJsonOpcoes += "[\"status " + dispositivo.pinos[c].nome + "\"]";
      } else {
        keyboardJsonOpcoes += "[\"status " + dispositivo.pinos[c].nome + "\"";
        keyboardJsonOpcoes += ", \"ligar " + dispositivo.pinos[c].nome + "\"";
        keyboardJsonOpcoes += ", \"desligar " + dispositivo.pinos[c].nome + "\"]";
      }
    }
    c++;
  }

  if (!primeiro) {   
    keyboardJsonOpcoes += ", ";
  }

  keyboardJsonOpcoes += "[\"adicionar dispositivo\", \"remover dispositivo\"]";

  //Se for o administrador (primeiro usuário) permite também remover usuários
  if (dispositivo.usuarios[0].chat_id == chat_id) {
    keyboardJsonOpcoes += ", [\"definir senha\", \"remover usuario\", \"listar usuarios\"]";
  } else {
    keyboardJsonOpcoes += ", [\"definir senha\", \"listar usuarios\"]";
  }
  keyboardJsonOpcoes += "]";
  
  return  keyboardJsonOpcoes;
}


void enviarMensagem(UniversalTelegramBot bot_user, String chat_id, String mensagem) {
    bot_user.sendMessage(chat_id, mensagem, "Markdown");
}

void enviarMensagemOpcoes(UniversalTelegramBot bot_user, String chat_id, String mensagem, String keyboardJsonOpcoes) {
  bot_user.sendMessageWithReplyKeyboard(chat_id, mensagem, "", keyboardJsonOpcoes, true);
}

String getNomesUsuarios() {
  String nomes = "";
  int c = 0;
  while(c < sizeof(dispositivo.usuarios)/sizeof(Usuario)) {
    if (dispositivo.usuarios[c].nome != "") {
      nomes += "\n" + dispositivo.usuarios[c].nome;
    }
    c++;
  }
  return nomes;
}
/*
ETAPAS DO BOT
0 - Status inicial do bot
1 - Aguardando informar o Pino que será adicionado
2 - Aguardando informar o Nome do Pino
3 - Aguardando o pino que será removido
4 - Aguardando informar usuário para remoção
5 - Aguardando definição da senha de novo acesso
6 - Aguardando senha mestra
7 - Plataforma reiniciada*/
void mensagensBotPersonalizado(int numNewMessages, UniversalTelegramBot bot_usuario) {
    int i=0;
    String chat_id = String(bot_usuario.messages[i].chat_id);
    String text = bot_usuario.messages[i].text;
    String text_chat = text;
    String nome_usuario = bot_usuario.messages[i].from_name;
    
    if (nome_usuario == "") nome_usuario = "humano";

    // Tratamento para cada tipo de comando a seguir.
    text.toLowerCase();
    text = utf8ToLatin(text);
    
    //Caso não tenha nenhum usuário, adiciona o primeiro a interagir com o bot
    if (!possuiUsuario()) {      
      adicionarUsuario(nome_usuario, chat_id);
    }

    if (dispositivo.etapa_configuracao == 4) {
      //TODO: Salva o dispositivo no Banco de dados
      dispositivo.etapa_configuracao = 5;
    }

    String keyboardJsonOpcoes = recuperarOpcoes(chat_id);    

    if (!permiteInteragir(chat_id)) {
      if (dispositivo.senha_acesso == text) {        
        adicionarUsuario(nome_usuario, chat_id);
        enviarMensagem(bot_usuario, chat_id, "Olá "+ nome_usuario + " agora podemos conversar, se não souber por onde começar basta escrever 'ajuda'.");
        dispositivo.senha_acesso = "";
      } else {
        enviarMensagem(bot_usuario, chat_id, "Olá "+ nome_usuario + ", tudo certo?\nO meu humano não gosta que eu fale com estranhos, por favor, informe a senha de acesso para que possamos conversar.");
      }      
      return;
    }

    int posicao_usuario = getPosicaoUsuario(chat_id);
   
    if (text == "cancelar") {
      enviarMensagem(bot_usuario, chat_id, "Feito, cancelei aqui pra você!");
      dispositivo.usuarios[posicao_usuario].etapa = 0;
    }
    else if (text == "ajuda" || text == "me ajude") {
      enviarMensagemOpcoes(bot_usuario, chat_id, "Opa, vamos lá.\nPara começar, você pode utilizar os seguintes comandos para que possamos ir conversando.", keyboardJsonOpcoes);
    }
    else if (text == "chat id") {
      enviarMensagem(bot_usuario, chat_id, "Olá " + nome_usuario + " o número do seu chat id é: " + chat_id);
      dispositivo.usuarios[posicao_usuario].etapa = 0;
    }
    else if (text == "adicionar dispositivo") {
      String keyboardJson = keyboardPinosDisponiveis(dispositivo.mac);
      //Armazena o mac do dispostivo
      pino_configuracao.mac_dispositivo = dispositivo.mac;
      enviarMensagemOpcoes(bot_usuario, chat_id, "Por favor, selecione em qual pino será ligado o dispositivo.", keyboardJson);          
      dispositivo.usuarios[posicao_usuario].etapa = 1;
    } 
    else if (text == "remover dispositivo") { 
      if (!possuiPino(dispositivo.mac)) {
        enviarMensagem(bot_usuario, chat_id, "Você ainda não possui nenhum dispositivo conectado. É melhor adicionar antes de remover.");
        dispositivo.usuarios[posicao_usuario].etapa = 0;
      } else {         
        String keyboardJson = keyboardNomesPinosEmUso(); 
        enviarMensagemOpcoes(bot_usuario, chat_id, "Por favor, informe qual dispositivo você gostaria de remover.", keyboardJson);
        dispositivo.usuarios[posicao_usuario].etapa = 3;        
      }      
    }
    else if (text.indexOf("desligar") >= 0) {
      String nome_pino = text;
      nome_pino = nome_pino.substring(8);
      nome_pino.trim();
      
      if (nome_pino == "") {
        enviarMensagem(bot_usuario, chat_id, "Você esqueceu de informar o nome do dispositivo.");  
      }
      else {
        int posicao_pino = getPosicaoPinoByNome(nome_pino);  
        if (dispositivo.pinos[posicao_pino].nome_digital == "a0") {
          enviarMensagem(bot_usuario, chat_id, "O dispositivo " + nome_pino + " não pode ser desligado.");
        } else {                
          if (posicao_pino >= 0) {
              int numero_pino = getNumeroPinoByNomeDigital(dispositivo.pinos[posicao_pino].nome_digital);                  
              digitalWrite(numero_pino, LOW);            
              enviarMensagem(bot_usuario, chat_id, "Desliguei " +nome_pino);
          } else {
            enviarMensagem(bot_usuario, chat_id, "Não existe nenhum dispositivo com o nome informado.");
          }
        }
      } 
    }
    else if (text.indexOf("ligar") >= 0) {
      String nome_pino = text;
      nome_pino = nome_pino.substring(5);
      nome_pino.trim();
      if (nome_pino == "") {
        enviarMensagem(bot_usuario, chat_id, "Você esqueceu de informar o nome do dispositivo.");  
      }
      else {
        int posicao_pino = getPosicaoPinoByNome(nome_pino);
        if (dispositivo.pinos[posicao_pino].nome_digital == "a0") {
          enviarMensagem(bot_usuario, chat_id, "O dispositivo " + nome_pino + " não pode ser ligado.");
        } else {          
          if (posicao_pino >= 0) {
              int numero_pino = getNumeroPinoByNomeDigital(dispositivo.pinos[posicao_pino].nome_digital);   
              digitalWrite(numero_pino, HIGH);
              enviarMensagem(bot_usuario, chat_id, "Liguei " +nome_pino);
          } else {
            enviarMensagem(bot_usuario, chat_id, "Não existe nenhum dispositivo com o nome informado.");
          }
        }
      }
    }
    else if (text == "reiniciar plataforma") {      
      enviarMensagem(bot_usuario, chat_id, "Para prosseguir, informe a senha mestra");
      dispositivo.usuarios[posicao_usuario].etapa = 7; 
    }
    else if (text.indexOf("status") >= 0) {
      String nome_pino = text;
      nome_pino = nome_pino.substring(6);
      nome_pino.trim();
      if (nome_pino == "") {
        enviarMensagem(bot_usuario, chat_id, "Você esqueceu de informar o nome do dispositivo.");  
      }
      else {
        int posicao_pino = getPosicaoPinoByNome(nome_pino);
        if (posicao_pino >= 0) {
            int numero_pino = getNumeroPinoByNomeDigital(dispositivo.pinos[posicao_pino].nome_digital);
            if (dispositivo.pinos[posicao_pino].nome_digital == "a0") {
              int valor_lido  = analogRead(numero_pino);
              enviarMensagem(bot_usuario, chat_id, "O valor lido de " + nome_pino + " é: " + valor_lido);
            } else {
              bool ligado = digitalRead(numero_pino);
              if (ligado) {
                enviarMensagem(bot_usuario, chat_id, nome_pino + " está ligado!");
              } else {
                enviarMensagem(bot_usuario, chat_id, nome_pino + " está desligado!");
              }
            }
        } else {
          enviarMensagem(bot_usuario, chat_id, "Não existe nenhum dispositivo com o nome informado.");
        }
      }      
    }
    else if (text == "definir senha" && dispositivo.usuarios[posicao_usuario].etapa == 0) { 
      enviarMensagem(bot_usuario, chat_id, "Por favor, informe qual será a senha para um novo acesso.\nPor segurança, ela ela somente pode ser utilizada uma vez, após o uso, você poderá gerar uma nova senha novamente.");   
      dispositivo.usuarios[posicao_usuario].etapa = 5;               
    }
    else if (text == "remover usuario" && dispositivo.usuarios[posicao_usuario].etapa == 0) { 
          if (dispositivo.usuarios[0].chat_id == chat_id) {
            enviarMensagem(bot_usuario, chat_id, "Vamos lá! Só me informe o nome de quem nós vamos remover.");
            dispositivo.usuarios[posicao_usuario].etapa = 4;
          } else {
            enviarMensagem(bot_usuario, chat_id, "Hey " + nome_usuario + " você não pode fazer isso!");
          }          
    }   
    else if (text == "listar usuarios" && dispositivo.usuarios[posicao_usuario].etapa == 0) {
       String nomesUsuarios = getNomesUsuarios();
       enviarMensagem(bot_usuario, chat_id, "Atualmente eu posso conversar com as seguintes pessoas:\n" + nomesUsuarios);
    }
    else if (dispositivo.usuarios[posicao_usuario].etapa == 0 && !possuiPino()) {
      String keyboardJson = keyboardPinosDisponiveis(dispositivo.mac);
      enviarMensagemOpcoes(bot_usuario, chat_id, "Olá " + nome_usuario + " para começarmos, por favor, informe em qual pino será ligado o dispositivo.", keyboardJson);          
      dispositivo.usuarios[posicao_usuario].etapa = 1;      
    }    
    else if (dispositivo.usuarios[posicao_usuario].etapa == 1) {     
      int numero_pino = getNumeroPinoEmUsoByNomeDigital(text, pino_configuracao.mac_dispositivo);
      //Caso o valor informado seja inválido
      if (numero_pino == -2) {
        String keyboardJson = keyboardPinosDisponiveis(pino_configuracao.mac_dispositivo);
        enviarMensagemOpcoes(bot_usuario, chat_id, "O pino está errado, para facilitar escolha o pino dentre as opções.", keyboardJson);          
      }
      else if (numero_pino == -1) {
        String keyboardJson = keyboardPinosDisponiveis(pino_configuracao.mac_dispositivo);
        enviarMensagemOpcoes(bot_usuario, chat_id, "O Pino informado já está em uso, por favor, escolha outro.", keyboardJson);          
      }
      else {
        enviarMensagem(bot_usuario, chat_id, "Informe um nome para o dispositivo.\nEste nome será utilizado sempre que você quiser ligá-lo ou desligá-lo.");
        pino_configuracao.nome_digital = text;
        dispositivo.usuarios[posicao_usuario].etapa = 2;
      }      
    }
    else if (dispositivo.usuarios[posicao_usuario].etapa == 2) {
      if (isPinoEmUsoByNome(text)) {     
        enviarMensagem(bot_usuario, chat_id, "O nome que você me disse não pode ser utilizado porque ele pertence a outro dispositivo\nPor favor, escolha outro.");
      } else {
        pino_configuracao.nome = text;
        //Exibe a mensagem antes que o pino de configuração seja limpo
        enviarMensagem(bot_usuario, chat_id, "Boa escolha. Já configurei o dispositivo pra você.\n\nPino:" + pino_configuracao.nome_digital + "\nNome: " + pino_configuracao.nome);
        adicionarPino();
        dispositivo.usuarios[posicao_usuario].etapa = 0;
      }
    }
    else if (dispositivo.usuarios[posicao_usuario].etapa == 3) {
      if (!isPinoEmUsoByNome(text)) {    
        String keyboardJson = keyboardNomesPinosEmUso();
        enviarMensagemOpcoes(bot_usuario, chat_id, "O nome que você me disse não está em uso\nPor favor, informe o nome do dispositivo corretamente.", keyboardJson); 
      } else {
        //Exibe a mensagem antes que o pino de configuração seja limpo
        enviarMensagem(bot_usuario, chat_id, "Beleza, já removi o dispositivo " + text);    
                
        removerPino(text);
        dispositivo.usuarios[posicao_usuario].etapa = 0;        
      }
    }    
    else if (dispositivo.usuarios[posicao_usuario].etapa == 4) {
      if (removerUsuario(text)) {
        enviarMensagem(bot_usuario, chat_id, "Feito. Removi o usuário " + text);        
      } 
      else {
        String nome_usuario = dispositivo.usuarios[0].nome;
        nome_usuario.toLowerCase();
        nome_usuario = utf8ToLatin(nome_usuario);
        if (text == nome_usuario) {
          enviarMensagem(bot_usuario, chat_id, "Não é possível remover o usuário principal!");
        }
        else {
          enviarMensagem(bot_usuario, chat_id, "Não encontrei ninguém com o nome: " + text);
        } 
      }
      dispositivo.usuarios[posicao_usuario].etapa = 0;
    } 
    else if (dispositivo.usuarios[posicao_usuario].etapa == 5) {
      dispositivo.senha_acesso = text;
      enviarMensagem(bot_usuario, chat_id, "Ok. Salvei a senha, agora peça para que a outra pessoa informe essa senha informe essa senha em nossa nova conversa.");
      dispositivo.usuarios[posicao_usuario].etapa = 0;       
    }
    else if (dispositivo.usuarios[posicao_usuario].etapa == 6) {
      if (text == dispositivo.senha_mestra) {
        String keyboardJson = "[[\"Sim\", \"Não\"]]";
        enviarMensagemOpcoes(bot_usuario, chat_id, "Deseja realmente reiniciar a plataforma? Lembrando que todos os dados salvos serão perdidos.", keyboardJson);
        dispositivo.usuarios[posicao_usuario].etapa = 7;
      } else {
        enviarMensagem(bot_usuario, chat_id, "Senha incorreta! Para prosseguir, informe a senha mestra. Lembrando que você pode 'cancelar' esse processo."); 
      }
    }
    else if (dispositivo.usuarios[posicao_usuario].etapa == 7) {
      if (text == "sim" || text == "/sim") {
        reiniciarDispositivo();
        enviarMensagem(bot_usuario, chat_id, "Já que você pediu! Eu apaguei todas as informações salvas e não podemos mais conversar. Agora reinicie todo o processo com o Bot de configuração.");
      } else { 
        //Caso a resposta seja negativa, volta para a etapa 0
        enviarMensagem(bot_usuario, chat_id, "Beleza, operação cancelada.");
        dispositivo.usuarios[posicao_usuario].etapa = 0;
      }      
    }        
    else {      
      enviarMensagemOpcoes(bot_usuario, chat_id, "Você pode utilizar os seguintes comandos para que possamos interagir.", keyboardJsonOpcoes);
      dispositivo.usuarios[posicao_usuario].etapa = 0;
    }
}

String keyboardNomesPinosEmUso() {
  String keyboardJson = "";
  int c = 0;
  
    while(c < sizeof(dispositivo.pinos)/sizeof(Pino)) {
      if (dispositivo.pinos[c].nome != "") {
          if (keyboardJson != "") {
            keyboardJson += ", ";
          }
          keyboardJson += "\"" + dispositivo.pinos[c].nome + "\"";
      }
      c++;
    }
 

  if (keyboardJson != "") {
    keyboardJson = "[[" + keyboardJson + "]]";
  }
  
  return keyboardJson;
}

String keyboardPinosEmUso(String mac_dispositivo) {
  return keyboardPinosPorSituacao(true, mac_dispositivo);
}

String keyboardPinosDisponiveis(String mac_dispositivo) {
  return keyboardPinosPorSituacao(false, mac_dispositivo);
}

String keyboardPinosPorSituacao(bool em_uso, String mac_dispositivo) {
  String keyboardJson = "";

  if ((isPinoEmUsoByNomeDigital("d0", mac_dispositivo) && em_uso) || (!isPinoEmUsoByNomeDigital("d0", mac_dispositivo) && !em_uso)) {    
    keyboardJson += "\"D0\"";
  }
  if ((isPinoEmUsoByNomeDigital("d1", mac_dispositivo) && em_uso) || (!isPinoEmUsoByNomeDigital("d1", mac_dispositivo) && !em_uso)) {
    if (keyboardJson != "") {
      keyboardJson += ", ";
    }
    keyboardJson += "\"D1\"";
  }
  if ((isPinoEmUsoByNomeDigital("d2", mac_dispositivo) && em_uso) || (!isPinoEmUsoByNomeDigital("d2", mac_dispositivo) && !em_uso)) {
    if (keyboardJson != "") {
      keyboardJson += ", ";
    }
    keyboardJson += "\"D2\"";
  }
  if ((isPinoEmUsoByNomeDigital("d3", mac_dispositivo) && em_uso) || (!isPinoEmUsoByNomeDigital("d3", mac_dispositivo) && !em_uso)) {
    if (keyboardJson != "") {
      keyboardJson += ", ";
    }
    keyboardJson += "\"D3\"";
  }
  if ((isPinoEmUsoByNomeDigital("d4", mac_dispositivo) && em_uso) || (!isPinoEmUsoByNomeDigital("d4", mac_dispositivo) && !em_uso)) {
    if (keyboardJson != "") {
      keyboardJson += ", ";
    }
    keyboardJson += "\"D4\"";
  }
  if ((isPinoEmUsoByNomeDigital("d5", mac_dispositivo) && em_uso) || (!isPinoEmUsoByNomeDigital("d5", mac_dispositivo) && !em_uso)) {
    if (keyboardJson != "") {
      keyboardJson += ", ";
    }
    keyboardJson += "\"D5\"";
  }
  if ((isPinoEmUsoByNomeDigital("d6", mac_dispositivo) && em_uso) || (!isPinoEmUsoByNomeDigital("d6", mac_dispositivo) && !em_uso)) {
    if (keyboardJson != "") {
      keyboardJson += ", ";
    }
    keyboardJson += "\"D6\"";
  }
  if ((isPinoEmUsoByNomeDigital("d7", mac_dispositivo) && em_uso) || (!isPinoEmUsoByNomeDigital("d7", mac_dispositivo) && !em_uso)) {
    if (keyboardJson != "") {
      keyboardJson += ", ";
    }
    keyboardJson += "\"D7\"";
  }
  if ((isPinoEmUsoByNomeDigital("d8", mac_dispositivo) && em_uso) || (!isPinoEmUsoByNomeDigital("d8", mac_dispositivo) && !em_uso)) {
    if (keyboardJson != "") {
      keyboardJson += ", ";
    }
    keyboardJson += "\"D8\"";
  }
  if ((isPinoEmUsoByNomeDigital("a0", mac_dispositivo) && em_uso) || (!isPinoEmUsoByNomeDigital("a0", mac_dispositivo) && !em_uso)) {
    if (keyboardJson != "") {
      keyboardJson += ", ";
    }
    keyboardJson += "\"A0\"";
  }
  
  keyboardJson = "[[" + keyboardJson + "]]";

  return keyboardJson;
}

bool isPinoEmUsoByNome(String nome_pino) {  
  int c = 0;

  while(c < sizeof(dispositivo.pinos)/sizeof(Pino)) {
    if (dispositivo.pinos[c].nome == nome_pino) {
      return true;
      c = sizeof(dispositivo.pinos)/sizeof(Pino);
    }
    c++;
  }

  return false;
}

bool isPinoEmUsoByNomeDigital(String nome_digital, String mac_dispositivo)
{  
  bool pinoEmUso = false;
  int c = 0;
 
  while(c < sizeof(dispositivo.pinos)/sizeof(Pino)) {
    if (dispositivo.pinos[c].nome_digital == nome_digital) {
      pinoEmUso = true;
      c = sizeof(dispositivo.pinos)/sizeof(Pino);
    }
    c++;
  }
  
  return pinoEmUso;  
}

int getPosicaoPinoByNome(String nome) { 
  int c = 0;
  while(c < sizeof(dispositivo.pinos)/sizeof(Pino)) {
    if (dispositivo.pinos[c].nome == nome) {
      return c;
    }
    c++;
  }
  return -1;
}

int getNumeroPinoEmUsoByNomeDigital(String nome_pino, String mac_dispositivo) {

  int numero_pino = getNumeroPinoByNomeDigital(nome_pino);
  if (numero_pino >= 0) {
    if (isPinoEmUsoByNomeDigital(nome_pino, mac_dispositivo))
    {
      return -1;    
    }
  }
  return numero_pino;
}

int getNumeroPinoByNomeDigital(String nome_pino) {
  int numero_pino = -2;
  
  if (nome_pino == "d0") {
    numero_pino = PIN_D0;
  }
  else if (nome_pino == "d1") {
    numero_pino = PIN_D1;
  }
  else if (nome_pino == "d2") {
    numero_pino = PIN_D2;
  }
  else if (nome_pino == "d3") {
    numero_pino = PIN_D3;
  }
  else if (nome_pino == "d4") {
    numero_pino = PIN_D4;
  }
  else if (nome_pino == "d5") {
    numero_pino = PIN_D5;
  }
  else if (nome_pino == "d6") {
    numero_pino = PIN_D6;
  }
  else if (nome_pino == "d7") {
    numero_pino = PIN_D7;
  }
  else if (nome_pino == "d8") {
    numero_pino = PIN_D8;
  }
  else if (nome_pino == "a0") {
    numero_pino = PIN_A0;
  }
  
  return numero_pino;
}

void adicionarPino() {
  int c = 0;  
  //Adiciona no meu dispositivo
  
  while(c < sizeof(dispositivo.pinos)/sizeof(Pino)) {
    if (dispositivo.pinos[c].nome_digital == "") {
      dispositivo.pinos[c].nome = pino_configuracao.nome;
      dispositivo.pinos[c].nome_digital = pino_configuracao.nome_digital;

      c = sizeof(dispositivo.pinos)/sizeof(Pino);
    }
    c++;
  }

  //Reinicia o pino de configuracao
  pino_configuracao.nome = "";
  pino_configuracao.nome_digital = "";
  pino_configuracao.mac_dispositivo = "";
}

bool possuiUsuario() {
  return dispositivo.usuarios[0].chat_id != ""; 
}

bool possuiPino(String mac_dispositivo) {
  return dispositivo.pinos[0].nome_digital != "";
}

bool possuiPino() {
  if (dispositivo.pinos[0].nome_digital != "") {
    return true;  
  }
 
  return false;
}

bool permiteInteragir(String chat_id) {
  int c = 0;
  bool permitir = false;
  int tamanho_array = sizeof(dispositivo.usuarios)/sizeof(Usuario);
  while(c < tamanho_array) {
    if (dispositivo.usuarios[c].chat_id == chat_id) {
        permitir = true;
        c = tamanho_array;
    }
    c++;
  }  
  return permitir;
}

int getPosicaoUsuario(String chat_id) {
  int c = 0;
  bool permitir = false;
  int tamanho_array = sizeof(dispositivo.usuarios)/sizeof(Usuario);
  while(c < tamanho_array) {
    if (dispositivo.usuarios[c].chat_id == chat_id) {
        return c;
    }
    c++;
  }  
  return -1;
}

void removerPino(String nome) {
  int c = 0;
  bool pino_removido = false;
  int tamanho_array = sizeof(dispositivo.pinos)/sizeof(Pino);
  while(c < tamanho_array) {
    if (dispositivo.pinos[c].nome == nome || pino_removido) {
        
        //Se for a ultima posicao, zera seus valores
        if (c == tamanho_array-1) {
          dispositivo.pinos[c].nome = "";
          dispositivo.pinos[c].nome_digital = "";
        } else {
          dispositivo.pinos[c].nome = dispositivo.pinos[c+1].nome;
          dispositivo.pinos[c].nome_digital = dispositivo.pinos[c+1].nome_digital;
        }
        pino_removido = true;
    }
    c++;
  }  
}

void adicionarUsuario(String nome_usuario, String chat_id) {
  int c = 0;
  
  while(c < sizeof(dispositivo.usuarios)/sizeof(Usuario)) {
    if (dispositivo.usuarios[c].nome == "") {
       dispositivo.usuarios[c].nome = nome_usuario;
       dispositivo.usuarios[c].chat_id = chat_id;
       c = sizeof(dispositivo.usuarios)/sizeof(Usuario);
    }
    c++;
  }  
}

bool removerUsuario(String nome) {
  if (nome == ""){
    return false;
  }  
  bool usuario_removido = false;
  int c = 1; //Começa da posição 1 para desconsiderar o usuário principal
  int tamanho_array = sizeof(dispositivo.usuarios)/sizeof(Usuario);
  while(c < tamanho_array) {
    //Coloca o nome do usuário em minísculo e remove os acentos para facilitar a comparação
    String nome_usuario = dispositivo.usuarios[c].nome;
    nome_usuario.toLowerCase();
    nome_usuario = utf8ToLatin(nome_usuario);
    if (nome_usuario == nome || usuario_removido) {
        
        //Se for a ultima posicao, zera seus valores
        if (c == tamanho_array-1) {
          dispositivo.usuarios[c].nome = "";
          dispositivo.usuarios[c].chat_id = "";
          dispositivo.usuarios[c].etapa = 0;
        } else {
          dispositivo.usuarios[c].nome = dispositivo.usuarios[c+1].nome;
          dispositivo.usuarios[c].chat_id = dispositivo.usuarios[c+1].chat_id;
          dispositivo.usuarios[c].etapa = dispositivo.usuarios[c+1].etapa;
        }
        usuario_removido = true;

        nome_usuario = dispositivo.usuarios[c].nome;
        nome_usuario.toLowerCase();
        nome_usuario = utf8ToLatin(nome_usuario);

        //Se o nome ainda continuar o mesmo (homônimo) - retorna uma posição para removê-lo na sequência
        if (nome_usuario == nome) {
          c--;
        }
    }
    c++;
  }

  return usuario_removido;
}



String utf8ToLatin(String texto) {
  texto.replace("u00c0", "a"); //À
  texto.replace("u00c1", "a"); //Á
  texto.replace("u00c2", "a"); //Â
  texto.replace("u00c3", "a"); //Ã
  texto.replace("u00c4", "a"); //Ä
  texto.replace("u00c5", "a"); //Å

  texto.replace("u00c7", "c"); //Ç

  texto.replace("u00c8", "e"); //È
  texto.replace("u00c9", "e"); //É
  texto.replace("u00ca", "e"); //Ê
  texto.replace("u00cb", "e"); //Ë
  
  texto.replace("u00cc", "i"); //Ì
  texto.replace("u00cd", "i"); //Í
  texto.replace("u00ce", "i"); //Î
  texto.replace("u00cf", "i"); //Ï

  texto.replace("u00d1", "n"); //Ñ

  texto.replace("u00d2", "o"); //Ò
  texto.replace("u00d3", "o"); //Ó
  texto.replace("u00d4", "o"); //Ô
  texto.replace("u00d5", "o"); //Õ
  texto.replace("u00d6", "o"); //Ö
  
  texto.replace("u00d9", "u"); //Ù
  texto.replace("u00da", "u"); //Ú
  texto.replace("u00db", "u"); //Û
  texto.replace("u00dc", "u"); //Ü
  
  texto.replace("u00dd", "y"); //Ý

  texto.replace("u00e0", "a"); //à
  texto.replace("u00e1", "a"); //á
  texto.replace("u00e2", "a"); //â
  texto.replace("u00e3", "a"); //ã
  texto.replace("u00e4", "a"); //ä
  texto.replace("u00e5", "a"); //å

  texto.replace("u00e7", "c"); //ç

  texto.replace("u00e8", "e"); //è
  texto.replace("u00e9", "e"); //é
  texto.replace("u00ea", "e"); //ê
  texto.replace("u00eb", "e"); //ë
  
  texto.replace("u00ec", "i"); //ì
  texto.replace("u00ed", "i"); //í
  texto.replace("u00ee", "i"); //î
  texto.replace("u00ef", "i"); //ï

  texto.replace("u00f1", "n"); //ñ

  texto.replace("u00f2", "o"); //ò
  texto.replace("u00f3", "o"); //ó
  texto.replace("u00f4", "o"); //ô
  texto.replace("u00f5", "o"); //õ
  texto.replace("u00f6", "o"); //ö
  
  texto.replace("u00f9", "u"); //ù
  texto.replace("u00fa", "u"); //ú
  texto.replace("u00fb", "u"); //û
  texto.replace("u00fc", "u"); //ü
  
  texto.replace("u00fd", "y"); //ý
  
  return texto;
}

void setupPins(){
  pinMode(PIN_D0, OUTPUT); 
  pinMode(PIN_D1, OUTPUT); 
  pinMode(PIN_D2, OUTPUT); 
  pinMode(PIN_D3, OUTPUT); 
  pinMode(PIN_D4, OUTPUT); 
  pinMode(PIN_D5, OUTPUT); 
  pinMode(PIN_D6, OUTPUT); 
  pinMode(PIN_D7, OUTPUT); 
  pinMode(PIN_D8, OUTPUT);
  pinMode(PIN_A0, INPUT);
}

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
  if (i < 5)
    result += ':';
  }
  return result;
}



void setup() {
  Serial.begin(115200);
  
  unsigned char mac[6];
  WiFi.macAddress(mac);
  clientMac += macToStr(mac);

  WiFiManager wifiManager;

  wifiManager.autoConnect("ESP8266");
  
  setupPins();

  dispositivo.mac = clientMac;
  
  lastTimeScan = millis();  
}

void loop() {  
  if (millis() > lastTimeScan + BOT_SCAN_MESSAGE_INTERVAL)  {  
    //Bot de configuração 
    if (dispositivo.etapa_configuracao < 5) {
      int numNewMessages = bot_configuracao.getUpdates(bot_configuracao.last_message_received + 1);  
      while(numNewMessages) {
        mensagensBotConfiguracao(numNewMessages);
        numNewMessages = bot_configuracao.getUpdates(bot_configuracao.last_message_received + 1);
      }
    }
    
    if (dispositivo.token_id != "") {
        UniversalTelegramBot bot_usuario(dispositivo.token_id, client);
        //Bot do usuário
        int numMessage = bot_usuario.getUpdates(bot_usuario.last_message_received + 1);  
        while(numMessage) {
          mensagensBotPersonalizado(numMessage, bot_usuario);
          numMessage = bot_usuario.getUpdates(bot_usuario.last_message_received + 1);
        } 
         
    }     
    lastTimeScan = millis();
  }
  yield();
  delay(10);
}


