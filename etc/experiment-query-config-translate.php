#!/usr/bin/php
<?php
# Include PEAR::Console_Getopt
require_once 'Console/Getopt.php';

$command = new Command;
print_r($command->command());

$config = new Config;
$config->load($command->config());
$config->parse();

print("DONE\n");
exit(0);


class Command {
  private $options;
  private $command;
  private $short_format_config = 'hc:s:t:';
  private $syntax_config = array('ccl', 'cql', 'pqf');


  public function __construct() {
    $args = Console_Getopt::readPHPArgv();

    if ( PEAR::isError($args) ) {
      fwrite(STDERR,$args->getMessage()."\n");
      exit(1);
    }
    
    // Compatibility between "php script.php" and "./script.php"
    if ( realpath($_SERVER['argv'][0]) == __FILE__ ) {
      $this->options 
        = Console_Getopt::getOpt($args, $this->short_format_config);
    } else {
      $this->options 
        = Console_Getopt::getOpt2($args, $this->short_format_config);
    }
    
    // Check for invalid options
    if ( PEAR::isError($this->options) ) {
      fwrite(STDERR, $this->options->getMessage()."\n");
      $this->help();
    }
    
    $this->command = array();
    
    // Loop through the user provided options
    foreach ( $this->options[0] as $option ) {
      switch ( $option[0] ) {
      case 'h':
        help();
        break;
      case 's':
        $this->command['syntax'] = $option[1];
        break;
      case 't':
        $this->command['transform'] = $option[1];
        break;
     case 'c':
       $this->command['config'] = $option[1];
       break;
      }
    }
    
    // Loop through the user provided options
    foreach ($this->options[1] as $argument ) {
      $this->command['query'] .= ' ' . $argument;
    }
  }

  
  public function help() {
    fwrite(STDERR, "  Usage:\n");
    fwrite(STDERR, "  ./experiment-query-config-translate.php -s syntax -t transform -c config.xml query\n");
    fwrite(STDERR, "  Experiment with general query configuration syntax and transformations.\n");
    fwrite(STDERR, "  -c config.xml XML config file\n");
    fwrite(STDERR, "  -s syntax     Syntax of source query language, 'ccl', 'cql', 'pqf'\n");
    fwrite(STDERR, "  -t transform  Syntax of transformed query language, 'ccl', 'cql', 'pqf'\n");
    fwrite(STDERR, "  -h            Display help\n");
    fwrite(STDERR, "  query         Valid query in specified syntax\n");  
    exit(0);
  }

  public function command() {
    return $this->command;
  }
  
  public function syntax() {
    return $this->command['syntax'];
  }
  
  public function transform() {
    return $this->command['transform'];
  }
  
  public function config() {
    return $this->command['config'];
  }

  public function query() {
    return $this->commamd['query'];
  }
  
}

class Config {
  private $xml_conf;

  public function load($xml_file){
    $this->xml_conf = @simplexml_load_file($xml_file) 
      or die("Unable to load XML config file '" . $xml_file ."'\n"); 
    $this->xml_conf->registerXPathNamespace('iq', 
                                            'http://indexdata.com/query');
  }

  public function parse(){
    //foreach ($this->xml_conf->xpath('//desc') as $desc) {
    //echo "$desc\n";

    $namespaces =  $this->xml_conf->getNamespaces(true);
    foreach ($namespaces as $ns){
      print("namespace '" . $ns . "'\n");
    } 


    foreach ($this->xml_conf->xpath('//iq:syntax') as $syntax){
      print("syntax '" . $syntax['name']  . "'\n");
    }
   
    
    
  }
  
}

